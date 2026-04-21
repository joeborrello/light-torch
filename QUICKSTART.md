# Quick Start Guide

Get your Motion Capture and Playback System up and running in 30 minutes.

## Prerequisites

### Software
- [ ] Arduino IDE (1.8.19 or later) — [Download](https://www.arduino.cc/en/software)
- [ ] STM32duino board support — Install via Arduino IDE Board Manager
- [ ] PlatformIO — [Install](https://platformio.org/install/ide?install=vscode)
- [ ] USB drivers for ST-Link and ESP32

### Hardware (per device)
- [ ] 1× Proffie V3.9 board
- [ ] 1× ESP32 development board
- [ ] 1× RGBW LED strip (60 LEDs)
- [ ] 1× Speaker (8Ω, 0.5-1W)
- [ ] 1× 5V power supply (2-3A)
- [ ] Jumper wires
- [ ] ST-Link V2 programmer (for Proffie)
- [ ] USB cable (for ESP32)

## Step 1: Wire the Hardware

### Proffie ↔ ESP32 (UART)
```
Proffie TX (pin 1)  →  ESP32 RX (GPIO16)
Proffie RX (pin 0)  →  ESP32 TX (GPIO17)
Proffie GND         →  ESP32 GND
```

### Proffie ↔ LED Strip
```
Proffie pin 5       →  LED Data
Proffie 5V          →  LED +5V
Proffie GND         →  LED GND
```

### Proffie ↔ Speaker
```
Proffie A0 (DAC)    →  Speaker +
Proffie GND         →  Speaker -
```

### Power
```
5V supply +         →  Proffie VIN, LED strip +5V
5V supply GND       →  Proffie GND, LED strip GND
ESP32 USB           →  Power via USB (or connect to Proffie 3.3V)
```

**⚠️ Important**: Do NOT power the LED strip from Proffie's 5V pin — use external 5V supply!

See `docs/wiring.md` for detailed diagrams.

## Step 2: Flash ESP32 Firmware (Both Devices)

1. **Connect ESP32** to your computer via USB
2. **Open terminal** in `esp32_firmware/` directory
3. **Flash firmware**:
   ```bash
   pio run -t upload
   ```
4. **Monitor serial output** to get MAC address:
   ```bash
   pio device monitor
   ```
5. **Note the MAC address** — you'll see something like:
   ```
   ESP32 MAC: 24:6F:28:AB:CD:EF
   ```
6. **Repeat for second ESP32** and note its MAC address too

## Step 3: Configure Peer MAC Addresses

1. **Open** `proffie_firmware/config.h`
2. **Find** the `PEER_MAC` definition (line ~41):
   ```cpp
   #define PEER_MAC {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF}
   ```
3. **Replace** with the OTHER device's ESP32 MAC address
   - Device 1 config → use Device 2's MAC
   - Device 2 config → use Device 1's MAC
4. **Adjust LED count** if needed (line ~33):
   ```cpp
   #define NUM_LEDS 60  // Change to match your strip
   ```

## Step 4: Flash Proffie Firmware (Both Devices)

### Arduino IDE Setup (First Time Only)
1. **Open Arduino IDE**
2. **Install STM32 board support**:
   - Go to: Tools → Board → Boards Manager
   - Search: "STM32"
   - Install: "STM32 MCU based boards" by STMicroelectronics
3. **Install libraries**:
   - Sketch → Include Library → Manage Libraries
   - Install: "Adafruit NeoPixel"

### Flash Firmware
1. **Open** `proffie_firmware/proffie_firmware.ino` in Arduino IDE
2. **Select board**:
   - Tools → Board → STM32 boards → Generic STM32L4 series
   - Tools → Board part number → STM32L433CC
3. **Select upload method**:
   - Tools → Upload method → STLink
4. **Connect ST-Link** to Proffie:
   ```
   ST-Link SWDIO  →  Proffie SWDIO
   ST-Link SWCLK  →  Proffie SWCLK
   ST-Link GND    →  Proffie GND
   ST-Link 3.3V   →  Proffie 3.3V
   ```
5. **Upload**:
   - Click "Upload" button (→)
   - Wait for "Upload complete"
6. **Repeat for second Proffie** (with its corresponding config)

## Step 5: Test the System

### Device 1 (Transmitter)
1. **Power on** Device 1
2. **Check serial output** (115200 baud):
   ```
   Proffie Motion Capture System v1.0
   ✓ LSM6DS3 initialized
   ✓ LED strip initialized
   ✓ Audio initialized
   ✓ UART initialized
   → RECORDING
   ```
3. **Move the device** — LEDs and sound should respond to motion
4. **Shake hard** (>2.5g) — should see:
   ```
   → TRANSMITTING (shake detected!)
   Transmitting 500 samples...
   .........
   Transmission complete!
   → RECEIVING
   ```

### Device 2 (Receiver)
1. **Power on** Device 2
2. **Wait for transmission** from Device 1
3. **Should see**:
   ```
   → PLAYBACK
   Playing back received motion...
   Playback complete (500 samples)
   → IDLE
   ```
4. **LEDs and sound** should replay Device 1's motion pattern

### Role Swap
- After playback, Device 2 becomes the transmitter
- Device 1 becomes the receiver
- Shake Device 2 to send its pattern back to Device 1

## Troubleshooting

### "LSM6DS3 init failed!"
- **Check**: I2C wiring (SDA=14, SCL=13)
- **Check**: LSM6DS3 is soldered on Proffie board
- **Try**: Power cycle the Proffie

### "No ACK for sample X"
- **Check**: UART wiring (TX→RX, RX→TX, GND)
- **Check**: ESP32 is powered and running
- **Check**: Baud rate is 115200 on both sides

### No ESP-NOW Communication
- **Check**: Both ESP32s have correct peer MAC addresses
- **Check**: ESP32s are within ~10m range for testing
- **Try**: Reflash ESP32 firmware and verify MAC addresses

### LEDs Not Lighting
- **Check**: Data pin connection (pin 5)
- **Check**: 5V power supply is connected and adequate
- **Try**: Lower `LED_BRIGHTNESS` in `config.h` to 64

### No Audio
- **Check**: Speaker wiring (A0 to speaker +, GND to speaker -)
- **Check**: Speaker impedance (should be 8Ω)
- **Try**: Test with headphones instead

## Next Steps

- Read `README.md` for full system documentation
- See `docs/wiring.md` for detailed wiring diagrams
- Adjust motion-to-color mapping in `led_patterns.cpp`
- Tune shake detection threshold in `config.h`
- Experiment with different recording durations

## Support

If you encounter issues:
1. Check serial output from both Proffie and ESP32
2. Verify all wiring connections
3. Confirm MAC addresses are correctly configured
4. Test each device independently before pairing

Happy motion capturing! 🎨🎵
