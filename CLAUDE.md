# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dual-device motion capture and playback system. Each device is a **Proffieboard V3.9** (STM32L452, onboard LSM6DS3 IMU) paired with an **Arduino Nano ESP32** wireless bridge. Motion performed on device A is recorded, transmitted wirelessly via ESP-NOW, and played back on device B with matching light and sound. Both devices run identical firmware.

## Build & Upload — No Automated Build System

Both firmwares are compiled and uploaded manually via **Arduino IDE**. There are no `make`, `cmake`, or CLI build commands.

**Proffie firmware:**
- Board package: Proffieboard V3 (install via Arduino IDE Boards Manager)
- Open: `ProffieOS-v8.10/ProffieOS/ProffieOS.ino`
- Board: Proffieboard V3.9
- Upload requires DFU mode — on Windows, Zadig must install WinUSB driver for the DFU device; double-tap RESET to enter bootloader

**ESP32 firmware:**
- Board package: Arduino Nano ESP32 (install via Arduino IDE Boards Manager)
- Open: `esp32_firmware/esp32_firmware.ino`
- **Tools → USB CDC On Boot → Enabled** (required — without this, Serial Monitor and USB upload don't work)
- Upload also requires Zadig WinUSB driver on Windows; double-tap RESET for bootloader

## Architecture

### Data Flow
```
[Proffie A IMU] → UART (Serial3) → [ESP32 A] → ESP-NOW → [ESP32 B] → UART (Serial3) → [Proffie B]
```
Both directions work identically — the same firmware runs on both devices.

### Proffie Firmware (`ProffieOS-v8.10/ProffieOS/`)

The entry point is `ProffieOS.ino`. The config file (`config/motion_capture_config.h`) is selected there via `#define CONFIG_FILE`. The prop (`props/saber_motion_capture_prop.h`) is included from the config via `CONFIG_PROP`.

**State machine** in `saber_motion_capture_prop.h`:
```
MC_IDLE → (motion > 80dps for 400ms) → MC_RECORDING
MC_RECORDING → (stationary 500ms) → MC_WAITING_DOCK
MC_WAITING_DOCK → (stable 3s) → MC_TRANSMITTING → MC_AWAITING_REPLY
MC_AWAITING_REPLY → (ESP32 forwards reply from peer) → MC_RECEIVING → MC_AWAIT_PICKUP
MC_AWAIT_PICKUP → (motion > 20dps for 400ms) → MC_PLAYBACK → MC_IDLE (g_post_playback=true)
```
All waiting states (MC_AWAITING_REPLY, MC_AWAIT_PICKUP) time out to MC_IDLE after 5 minutes.

**Blade style** is defined in `config/motion_capture_config.h` as template metaprogramming (`Layers<>`, `Mix<>`, `AlphaL<>`, `Pulsing<>`). Global bools/floats declared in the prop (`g_await_pickup`, `g_sent_flash`, `g_awaiting_reply`, `g_sync_flash`, `g_playback_speed`, `g_in_playback`) bridge prop state to blade style functors.

**IMU speed** — `fusor.swing_speed()` uses only Y+Z gyro axes: `sqrtf(gyro_y² + gyro_z²)`. `g_playback_speed` must match this exactly (X axis excluded).

**Audio** — `beeper.Beep()` generates pentatonic arpeggiation when no SD card is present (`SFX_swing.files_found() == 0`). When an SD card is present with `motion_capture/swing##.wav` files, ProffieOS plays those instead and the beeper is silenced. `SFX_swing.Select(idx)` is called each Loop() to preselect the file by speed.

**Sync mode** — holding the device upside down (dot product of filtered_accel with home_gravity_ below -0.7) for 3 seconds (from MC_IDLE or MC_AWAITING_REPLY only) toggles `sync_mode_`. In sync mode, MC_IDLE never starts recording — both devices play live simultaneously.

**UART packet format** (70 bytes): `0xAA | type | len_lo | len_hi | payload[64] | checksum | 0x55`. A dummy `0xFF` byte is sent before each packet to work around LPUART1 first-byte drop on STM32.

**Serial port mapping (critical):**
- `Serial3` = LPUART1 = **PC1(TX) / PC0(RX)** — the labeled TX/RX pads on the board
- Do not use Serial1 (USART1, PA9/PA10) or Serial2 (USART3, PB10/PB11)

**Modified ProffieOS file:** `functions/hold_peak.h` has an added `calculate()` method required for `HoldPeakF` to work in blade style templates.

### ESP32 Firmware (`esp32_firmware/`)

Pure relay — receives UART packets from Proffie, forwards via ESP-NOW; receives ESP-NOW packets from peer, forwards via UART. No motion processing.

- `uart_handler.h/.cpp` — packet framing/parsing on `Serial2` using Arduino pin numbers `D2` (RX) and `D3` (TX)
- `espnow_handler.h/.cpp` — ESP-NOW broadcast to `FF:FF:FF:FF:FF:FF`, channel 1 set explicitly after `esp_now_init()`
- `uart_protocol.h` — shared packet type constants

**Pin mapping (critical):** `Serial2.begin(baud, SERIAL_8N1, D2, D3)` uses Arduino pin labels, **not** GPIO numbers. D2=GPIO5, D3=GPIO6. Passing raw GPIO 5/6 silently routes to D5/D6 (GPIO8/9) — wrong pins.

**ESP-NOW SDK note:** This project uses ESP32 Arduino core `esp32s3-libs/3.3.8` (IDF 5.x). The send callback signature is `(const wifi_tx_info_t*, esp_now_send_status_t)` — **not** `(const uint8_t*, esp_now_send_status_t)` as in older cores. Do not change this signature.

**ESP-NOW MAC timing:** `WiFi.macAddress()` returns `00:00:00:00:00:00` until the WiFi stack initializes. `espnow_handler.cpp` waits for a valid MAC before calling `esp_now_init()`.

## Key Constants (in `saber_motion_capture_prop.h`)

| Constant | Value | Purpose |
|---|---|---|
| `MIN_SWING_SPEED` | 30 dps | Note/sound generation threshold |
| `RECORD_MIN_SPEED` | 20 dps | Recording and playback trigger |
| `MOTION_DEBOUNCE_MS` | 400 ms | Sustained motion required to trigger |
| `STATIONARY_THRESHOLD` | 10 dps | Below this = stationary |
| `SYNC_INVERT_THRESHOLD` | -0.7 | dot(filtered_accel, home_gravity) below this = upside down → sync toggle |
| `SYNC_HOLD_MS` | 3000 ms | Duration required to toggle sync |
| `DOCK_SETTLE_MS` | 3000 ms | Stationary time before transmitting |
| `MOTION_BUFFER_SIZE` | 1500 | Samples (30s at 50Hz) |

## SD Card Setup

FAT32, folder name `motion_capture/` (must match preset name in config). Files: `hum.wav` (looping drone), `swing01.wav`–`swing08.wav` (speed-indexed one-shots). Format: 44100 Hz, 16-bit, mono WAV. File count is detected automatically — `SFX_swing.files_found()`.

## Potential Revert Flag

**MC_AWAIT_PICKUP debounce** — the 80 dps / 400 ms debounce before pickup triggers playback was added alongside the recording debounce. If pickup-to-playback feels sluggish, revert just that case: replace the debounce block with `if (IsMoving())` and remove the `first_motion_time_` resets in MC_AWAIT_PICKUP.

## Diagnostic Mode

`#define UART_DIAGNOSTIC_MODE` in the prop enables periodic test packets, Serial Monitor logging, and a UART loopback test. Commented out for production. `#define UART_VERBOSE` in `esp32_firmware.ino` enables verbose UART packet logging on the ESP32 side.
