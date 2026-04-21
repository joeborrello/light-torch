# Wiring Diagram: Proffie V3.9 + ESP32 Motion Capture System

## PROFFIE V3.9 → ESP32 CONNECTIONS

### UART (Serial1)
- **TX pad (PA9)** → ESP32 GPIO16 (RX2)
- **RX pad (PA10)** → ESP32 GPIO17 (TX2)
- **GND** → ESP32 GND

### Power
- **BATT+** ← 3.7V Li-ion battery (positive terminal)
- **GND** ← Battery negative terminal
- **3.3V** → ESP32 VIN (if powering ESP32 from Proffie regulator)

### RGBW NeoPixel Strip
- **Data1 (PA7, pin 0)** → LED strip DIN
- **5V** (from boost converter) → LED strip VCC
- **BATT-** → LED strip GND

### Audio
- **Speaker+** → 4-8Ω speaker (positive terminal)
- **Speaker-** → 4-8Ω speaker (negative terminal)

### LSM6DS3 IMU (Onboard)
- **SDA** → Pin 14 (PA9) - I2C data
- **SCL** → Pin 13 (PA8) - I2C clock
- **I2C Address**: 0x6A (factory default)

## ESP32 Pinout

### UART (Serial2)
- **GPIO16 (RX2)** ← Proffie TX
- **GPIO17 (TX2)** → Proffie RX

### Power
- **VIN** ← Proffie 3.3V (or separate 3.3V supply)
- **GND** → Common ground with Proffie

### ESP-NOW (WiFi)
- Uses built-in WiFi radio (no external connections)

## Notes

1. **Power Budget**: Ensure battery can supply:
   - Proffie idle: ~50mA
   - ESP32 active (WiFi): ~150-250mA
   - LED strip: ~60mA per LED at full white (60 LEDs = 3.6A max!)
   - Speaker: ~500mA peak
   - **Recommended battery**: 3.7V 2000mAh+ Li-ion with 5A+ discharge rating

2. **LED Strip Current Limiting**: Consider adding a current-limiting resistor (220-470Ω) on the data line to prevent signal reflections.

3. **ESP32 Power**: If powering ESP32 from Proffie's 3.3V regulator, verify regulator can handle combined load (~200mA for ESP32 + Proffie logic).

4. **UART Level Shifting**: Proffie V3.9 uses 3.3V logic (STM32L433), compatible with ESP32 (3.3V). No level shifter needed.

5. **I2C Pull-ups**: LSM6DS3 onboard pull-ups should be sufficient. If communication issues occur, add external 4.7kΩ pull-ups to SDA/SCL.
