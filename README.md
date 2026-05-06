# Light Torch — Motion Capture & Playback System

A two-device system where motion performed on one device is wirelessly transmitted to and played back on a second device. Each device has a Proffieboard V3.9 (with onboard IMU) paired with an ESP32 for wireless communication. Both devices generate real-time light and sound in response to motion at all times.

## How It Works

1. Move device A — LEDs and audio respond live to motion intensity
2. Set it down and hold still — the motion sequence is automatically recorded and transmitted to device B via ESP-NOW
3. Device A shows a white flash on transmission, then a slow white pulse while waiting
4. Device B pulses red — pick it up to trigger playback of device A's motion
5. Both devices can record and transmit to each other indefinitely

---

## Hardware (per device)

| Component | Part |
|---|---|
| Main board | Proffieboard V3.9 (STM32L452, onboard LSM6DS3 IMU) |
| Wireless bridge | Arduino Nano ESP32 |
| LED strip | WS2812-compatible RGBW, 144 LEDs |
| Speaker | 8Ω recommended |
| Power | 3.7V LiPo battery |

---

## Wiring

### Proffie ↔ ESP32 (UART)

| Proffie pad | ESP32 pin | Notes |
|---|---|---|
| TX (PC1) | D2 | Use Arduino pin label D2, not raw GPIO |
| RX (PC0) | D3 | Use Arduino pin label D3, not raw GPIO |
| GND | GND | Must share ground |

> **Important:** Proffie Serial3 (LPUART1) maps to the labeled TX/RX pads (PC1/PC0). Do not use Serial1 or Serial2.

### Proffie → LED Strip

| Proffie | LED Strip |
|---|---|
| Data1 (bladePin 0) | DIN |
| Batt+ | +V |
| GND | GND |

### Speaker

Connect to Proffie **SPKR+** and **SPKR−** pads.

---

## Software Setup

### Proffie (ProffieOS)

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add the Proffieboard board package — follow the [ProffieOS setup guide](https://fredrik.hubbe.net/lightsaber/proffieos.html)
3. Select **Tools → Board → Proffieboard V3**
4. Open `ProffieOS-v8.10/ProffieOS/ProffieOS.ino`
5. Compile and upload

The config file (`config/motion_capture_config.h`) and prop file (`props/saber_motion_capture_prop.h`) are already referenced by the included ProffieOS build.

> **Windows DFU upload:** If the upload fails, use [Zadig](https://zadig.akeo.ie/) to install the WinUSB driver for the Proffieboard DFU device. Double-tap the RESET button to enter bootloader mode.

### ESP32 (Arduino Nano ESP32)

1. Install Arduino IDE (same instance as above is fine)
2. Add the Arduino Nano ESP32 board package via Boards Manager
3. **Tools → USB CDC On Boot → Enabled** (required for Serial Monitor)
4. Open `esp32_firmware/esp32_firmware.ino`
5. Compile and upload

> **Windows DFU upload:** Same Zadig WinUSB swap applies. Double-tap RESET to enter bootloader.

---

## SD Card Audio (Optional)

Without an SD card, a built-in beeper generates pentatonic arpeggiation in response to motion. With an SD card, WAV files replace the beeper.

**Format the card:** FAT32, ≤32 GB

**Folder structure** (folder name must match exactly):
```
motion_capture/
├── hum.wav          ← looping ambient drone
├── swing01.wav      ← slowest swing sound
├── swing02.wav
│   ...
└── swing08.wav      ← fastest swing sound
```

**WAV specs:** 44100 Hz, 16-bit, mono

The swing file played is selected by speed: slow motion → `swing01`, fast motion → `swing08`. The number of files is detected automatically — fewer than 8 is fine.

---

## Behavior Reference

### LED States

| State | LED |
|---|---|
| Idle | Slow RGBW color cycle |
| Moving | Red → blue gradient by speed |
| Just transmitted | Rapid white flash (2 s) |
| Awaiting reply | Slow white pulse |
| Message waiting for pickup | Rapid red pulse |
| Sync mode toggle | Solid green flash (500 ms) |

### Sync Mode

Sustained vigorous motion (>300 dps for 3 seconds) from idle toggles **sync mode** on or off with a green flash. In sync mode, devices play live simultaneously without recording or transmitting. Repeat the gesture to exit.

### Timeouts

All waiting states (awaiting reply, awaiting pickup) time out to idle after 5 minutes.

---

## Configuration

Key thresholds in `ProffieOS-v8.10/ProffieOS/props/saber_motion_capture_prop.h`:

| Constant | Default | Description |
|---|---|---|
| `MIN_SWING_SPEED` | 30 dps | Motion threshold for note/sound generation |
| `RECORD_MIN_SPEED` | 80 dps | Motion threshold to trigger recording |
| `MOTION_DEBOUNCE_MS` | 400 ms | Sustained motion required before recording starts |
| `SYNC_TRIGGER_SPEED` | 300 dps | Vigorous motion threshold for sync mode toggle |
| `SYNC_HOLD_MS` | 3000 ms | How long to sustain vigorous motion to toggle sync |
| `STATIONARY_THRESHOLD` | 10 dps | Below this = considered stationary |

---

## Repository Structure

```
light-torch/
├── ProffieOS-v8.10/
│   └── ProffieOS/
│       ├── ProffieOS.ino                    ← open this in Arduino IDE
│       ├── config/
│       │   └── motion_capture_config.h      ← blade style & preset
│       ├── props/
│       │   └── saber_motion_capture_prop.h  ← state machine & all behavior
│       └── functions/
│           └── hold_peak.h                  ← modified ProffieOS file
├── esp32_firmware/
│   ├── esp32_firmware.ino                   ← main ESP32 sketch
│   ├── uart_handler.h / .cpp                ← UART bridge (Proffie ↔ ESP32)
│   ├── espnow_handler.h / .cpp              ← ESP-NOW wireless
│   └── uart_protocol.h                      ← shared packet format
└── README.md
```
