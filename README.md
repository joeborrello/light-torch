# Motion Capture and Playback System

A dual-device system for capturing motion patterns via IMU and transmitting them wirelessly for playback on a second device.

## System Overview

**Device 1 (Transmitter):**
- Proffie V3.9 board with LSM6DS3 IMU
- Records 10-15 seconds of motion data
- Generates real-time LED (RGBW) and audio patterns based on motion
- Transmits recorded sequence via ESP32 (ESP-NOW) when user shakes the device
- Switches to receive mode after transmission

**Device 2 (Receiver):**
- Identical hardware setup
- Receives motion sequence via ESP32
- Plays back LED and audio patterns
- Switches to transmit mode after playback

## Hardware Requirements

### Per Device:
- 1x Proffie V3.9 board (STM32L433 with onboard LSM6DS3 IMU)
- 1x ESP32 development board
- 1x RGBW LED strip (WS2812-compatible, 60 LEDs recommended)
- 1x Speaker (4-8Ω)
- 1x 3.7V Li-ion battery
- Jumper wires

## Wiring

See `docs/wiring.md` for complete wiring diagrams.

**Quick Reference:**

**Proffie → ESP32:**
- TX (PA9, pin 9) → ESP32 GPIO16 (RX2)
- RX (PA10, pin 10) → ESP32 GPIO17 (TX2)
- GND → ESP32 GND

**Proffie → LED Strip:**
- Data1 (PA7, pin 0) → LED DIN
- 5V → LED VCC
- GND → LED GND

**Proffie → Speaker:**
- Speaker+ / Speaker- terminals → 4-8Ω speaker

**LSM6DS3 (onboard):**
- I2C: SDA=pin 14, SCL=pin 13

## Software Setup

### Proffie Firmware

1. Install Arduino IDE with STM32 board support
2. Open `proffie_firmware/proffie_firmware.ino`
3. Install required libraries:
   - Wire (I2C)
   - Adafruit_NeoPixel (for RGBW LEDs)
4. Select board: **STM32L4 → Generic STM32L4 series → Proffie V3**
5. Upload to Proffie board

### ESP32 Firmware

1. Install PlatformIO (VS Code extension or CLI)
2. Navigate to `esp32_firmware/`
3. Run: `platformio run --target upload`
4. Or open in PlatformIO IDE and click Upload

## Configuration

### Proffie (`proffie_firmware/config.h`):
- `NUM_LEDS`: Number of LEDs in your strip (default: 60)
- `LED_BRIGHTNESS`: LED brightness 0-255 (default: 128)
- `RECORDING_DURATION_SEC`: Recording length in seconds (default: 10)
- `SHAKE_THRESHOLD_G`: Shake detection sensitivity (default: 2.5g)

### ESP32 (`esp32_firmware/platformio.ini`):
- No configuration needed for broadcast mode
- For paired mode, update `ESP32_PEER_MAC` in both devices' config

## Usage

1. **Power on both devices**
   - Both start in RECORDING mode automatically

2. **Record motion on Device 1**
   - Move the device around
   - LEDs and audio respond in real-time to motion
   - Recording buffer holds 10 seconds (rolling window)

3. **Transmit to Device 2**
   - Shake Device 1 rapidly (>2.5g acceleration)
   - Device 1 transmits recorded sequence via ESP-NOW
   - Device 1 enters RECEIVING mode

4. **Playback on Device 2**
   - Device 2 receives the sequence
   - Plays back the same LED and audio patterns
   - Device 2 enters RECORDING mode after playback

5. **Repeat**
   - Device 2 can now record and transmit back to Device 1

## LED Patterns

- **Idle**: Slow breathing effect (blue)
- **Recording**: Pulsing green
- **Transmitting**: Fast blinking yellow
- **Receiving**: Slow blinking cyan
- **Playback**: Motion-mapped colors (blue → green → yellow → red)

## Audio Patterns

- Tone frequency mapped to motion intensity: 200-2000 Hz
- Higher motion = higher pitch

## Troubleshooting

**LSM6DS3 init failed:**
- Check I2C wiring (SDA=14, SCL=13)
- Verify I2C address is 0x6A (SA0 pin grounded on Proffie V3.9)

**ESP32 not communicating:**
- Check UART wiring (TX→RX, RX→TX, GND→GND)
- Verify baud rate is 115200 on both sides
- Check Serial Monitor for UART errors

**LEDs not working:**
- Verify LED_DATA_PIN matches your wiring (default: pin 0 = PA7)
- Check 5V power supply to LED strip
- Ensure common ground between Proffie and LED strip

**No transmission between devices:**
- Both ESP32s must be powered on
- Check WiFi channel (default: channel 1)
- Verify ESP-NOW is initialized (check ESP32 serial output)

## File Structure

```
torch-project/
├── proffie_firmware/
│   ├── proffie_firmware.ino    # Main state machine
│   ├── config.h                # Configuration constants
│   ├── lsm6ds3_driver.h/cpp    # IMU driver
│   ├── shake_detector.h/cpp    # Shake detection
│   ├── motion_buffer.h/cpp     # Motion data buffer
│   ├── led_patterns.h/cpp      # LED pattern generator
│   ├── audio_tones.h/cpp       # Audio tone generator
│   └── uart_comm.h/cpp         # UART communication
├── esp32_firmware/
│   ├── esp32_firmware.ino      # Main ESP32 sketch
│   ├── uart_handler.h/cpp      # UART handler
│   ├── espnow_handler.h/cpp    # ESP-NOW handler
│   └── platformio.ini          # PlatformIO config
├── common/
│   └── uart_protocol.h         # Shared UART protocol
├── docs/
│   └── wiring.md               # Wiring diagrams
└── README.md                   # This file
```

## Technical Details

- **IMU Sampling**: 50 Hz (20ms intervals)
- **Motion Buffer**: 500 samples (10 seconds @ 50Hz) = ~8KB RAM
- **UART Protocol**: 115200 baud, packet-based with checksums
- **ESP-NOW**: Broadcast mode (FF:FF:FF:FF:FF:FF)
- **LED Update Rate**: 50 Hz (synchronized with IMU)
- **Audio Sample Rate**: Continuous tone generation via DAC

## License

MIT License - see LICENSE file for details
