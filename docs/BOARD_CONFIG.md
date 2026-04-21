# Board-Specific Configuration

## Device 1 (Data1)
- **LED Data Pin**: Data1 (Pin 2 on Proffie V3.9)
- **Config Setting**: `#define LED_DATA_PIN 2` in `proffie_firmware/config.h`

## Device 2 (Data2)
- **LED Data Pin**: Data2 (Pin 3 on Proffie V3.9)
- **Config Setting**: Change to `#define LED_DATA_PIN 3` in `proffie_firmware/config.h` before uploading to the second board

## Arduino Nano ESP32 Configuration
Both ESP32 boards use:
- **RX Pin**: RX0 (Pin 0)
- **TX Pin**: TX1 (Pin 1)
- **Status LED**: Pin 13 (built-in LED)

## Before Uploading Firmware

### For Device 1:
1. Ensure `config.h` has `#define LED_DATA_PIN 2`
2. Upload `proffie_firmware.ino` to Proffie board
3. Upload `esp32_firmware.ino` to Arduino Nano ESP32

### For Device 2:
1. Change `config.h` to `#define LED_DATA_PIN 3`
2. Upload `proffie_firmware.ino` to Proffie board
3. Upload `esp32_firmware.ino` to Arduino Nano ESP32 (same firmware works for both)

## Wiring Between Proffie and ESP32

| Proffie V3.9 | Arduino Nano ESP32 |
|--------------|-------------------|
| TX (PA9)     | RX0 (Pin 0)       |
| RX (PA10)    | TX1 (Pin 1)       |
| GND          | GND               |
| 3.3V         | 3.3V (optional)   |

**Note**: The Arduino Nano ESP32 can be powered via USB or from the Proffie's 3.3V output (if current draw is acceptable).
