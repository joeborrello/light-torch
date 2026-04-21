# Motion Capture and Playback System

## Overview

This project implements a wireless motion capture and playback system using two identical devices, each consisting of:
- **Proffie V3.9** board (STM32-based with onboard LSM6DS3 IMU)
- **ESP32** module (for wireless communication via ESP-NOW)
- **RGBW LED strip** (for visual feedback)
- **Speaker** (for audio feedback)

## How It Works

1. **Device 1 (Transmitter Mode)**:
   - Records motion from the LSM6DS3 IMU at 50Hz for 10 seconds
   - Generates real-time light (RGBW LED) and sound patterns based on motion intensity
   - User performs a **fast shake gesture** to trigger transmission
   - Sends the recorded motion sequence to Device 2 via ESP-NOW

2. **Device 2 (Receiver Mode)**:
   - Receives the motion sequence from Device 1
   - Plays back the same light and sound patterns
   - Automatically switches to **Transmitter Mode** after playback

3. **Role Reversal**:
   - After transmission, Device 1 enters Receiver Mode
   - After playback, Device 2 enters Transmitter Mode
   - The devices continuously swap roles

## Project Structure

```
torch-project/
├── proffie_firmware/          # Proffie V3.9 firmware (Arduino)
│   ├── proffie_firmware.ino   # Main state machine
│   ├── config.h               # Configuration constants
│   ├── lsm6ds3_driver.h/cpp   # LSM6DS3 I2C driver
│   ├── shake_detector.h/cpp   # Shake gesture detection
│   ├── motion_buffer.h/cpp    # Motion data buffer (500 samples)
│   ├── led_patterns.h/cpp     # RGBW LED pattern generator
│   ├── audio_tones.h/cpp      # Audio tone generator
│   └── uart_comm.h/cpp        # UART communication with ESP32
│
├── esp32_firmware/            # ESP32 firmware (PlatformIO)
│   ├── platformio.ini         # PlatformIO configuration
│   ├── esp32_firmware.ino     # Main ESP32 sketch
│   ├── uart_handler.h/cpp     # UART bridge (Proffie ↔ ESP32)
│   └── espnow_handler.h/cpp   # ESP-NOW wireless communication
│
├── common/
│   └── uart_protocol.h        # Shared UART protocol definition
│
└── docs/
    ├── wiring.md              # Complete wiring diagrams
    └── README.md              # This file
```

## Hardware Requirements

### Per Device (x2)
- 1x Proffie V3.9 board
- 1x ESP32 development board (ESP32-WROOM or similar)
- 1x RGBW LED strip (WS2812-compatible, 60 LEDs recommended)
- 1x Speaker (4-8Ω, 1-3W)
- 1x 3.7V Li-ion battery (2000+ mAh recommended)
- Jumper wires for connections

## Software Requirements

### Proffie Firmware
- **Arduino IDE** 1.8.x or 2.x
- **STM32duino** board support package
- **Adafruit NeoPixel** library (for RGBW LEDs)

### ESP32 Firmware
- **PlatformIO** (recommended) or Arduino IDE
- **ESP32 board support** (Espressif Systems)

## Getting Started

### 1. Flash Proffie Firmware
```bash
# Open proffie_firmware/proffie_firmware.ino in Arduino IDE
# Select board: "Generic STM32F4 series" (Proffie V3.9 uses STM32F411)
# Upload to Proffie board via USB
```

### 2. Flash ESP32 Firmware
```bash
cd esp32_firmware
pio run --target upload
# Or open esp32_firmware.ino in Arduino IDE and upload
```

### 3. Wire the Components
See `docs/wiring.md` for complete wiring diagrams.

### 4. Configure MAC Addresses
After flashing both ESP32 modules:
1. Open Serial Monitor on each ESP32 (115200 baud)
2. Note the MAC address printed at startup
3. Update `PEER_MAC` in `proffie_firmware/config.h` with the peer's MAC address
4. Re-flash the Proffie firmware

### 5. Test the System
1. Power on both devices
2. Move Device 1 to generate motion patterns
3. Perform a fast shake gesture on Device 1
4. Device 2 should receive and play back the sequence
5. Device 2 is now in transmit mode; repeat with Device 2

## Configuration

Edit `proffie_firmware/config.h` to customize:
- `NUM_LEDS` — Number of LEDs in your strip
- `LED_BRIGHTNESS` — LED brightness (0-255)
- `RECORDING_DURATION_SEC` — Recording length (default 10s)
- `SHAKE_THRESHOLD_G` — Shake sensitivity (default 2.5g)
- `PEER_MAC` — ESP32 peer MAC address

## Troubleshooting

### IMU Not Detected
- Check I2C wiring (SDA=14, SCL=13 on Proffie V3.9)
- Verify LSM6DS3 is soldered correctly (it's onboard)
- Check Serial Monitor for "LSM6DS3 init failed!" message

### ESP-NOW Transmission Fails
- Verify both ESP32 modules are on the same WiFi channel (default: 1)
- Check MAC addresses are correct in `config.h`
- Ensure devices are within ~100m range (line of sight)

### LEDs Not Lighting
- Check LED strip power (5V, sufficient current)
- Verify data pin connection (PA7 / pin 0 on Proffie)
- Confirm `NUM_LEDS` matches your strip length

### No Audio Output
- Check speaker polarity (doesn't matter for simple tones)
- Verify speaker impedance (4-8Ω)
- Ensure audio amplifier is enabled on Proffie

## Next Steps

- [ ] Implement battery level monitoring
- [ ] Add visual feedback for transmission status
- [ ] Optimize power consumption (sleep modes)
- [ ] Add persistent storage for multiple sequences
- [ ] Implement pairing mode for easier setup

## License

This project is open-source. Use and modify as needed.
