# Arduino IDE Build Guide

This guide explains how to build both the Proffie and ESP32 firmware using Arduino IDE on Windows.

## Prerequisites

1. **Arduino IDE 2.x** (recommended) or 1.8.x
   - Download from: https://www.arduino.cc/en/software

2. **Board Support Packages**:
   - STM32 boards (for Proffie)
   - ESP32 boards

3. **Required Libraries**:
   - Adafruit_NeoPixel (for RGBW LEDs)
   - Wire (built-in)

---

## Part 1: Install Board Support

### STM32 Boards (for Proffie V3.9)

1. Open Arduino IDE
2. Go to **File → Preferences**
3. In "Additional Boards Manager URLs", add:
   ```
   https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "STM32" and install **"STM32 MCU based boards" by STMicroelectronics**
6. Wait for installation to complete

### ESP32 Boards

1. In **File → Preferences**, add to "Additional Boards Manager URLs" (separate with comma if STM32 URL already there):
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Go to **Tools → Board → Boards Manager**
3. Search for "ESP32" and install **"esp32" by Espressif Systems**
4. Wait for installation to complete

---

## Part 2: Install Required Libraries

1. Go to **Tools → Manage Libraries** (or Sketch → Include Library → Manage Libraries)
2. Search for and install:
   - **Adafruit NeoPixel** by Adafruit
   - Wire (should be built-in, no installation needed)

---

## Part 3: Build Proffie Firmware

### Open the Project

1. Navigate to your project folder in Windows Explorer:
   ```
   \\wsl$\Ubuntu\mnt\c\Windows\system32\torch-project\proffie_firmware
   ```
   (Adjust the path based on your WSL distro name)

2. Double-click `proffie_firmware.ino` to open in Arduino IDE

### Configure Board Settings

1. Go to **Tools → Board** and select:
   - **STM32 boards groups → Generic STM32L4 series**

2. Set the following options under **Tools**:
   - **Board part number**: Generic L433CCTx
   - **Upload method**: STM32CubeProgrammer (SWD)
   - **USB support**: CDC (generic 'Serial' supersede U(S)ART)
   - **U(S)ART support**: Enabled (generic 'Serial')
   - **Optimize**: Smallest (-Os default)

3. Select your **Port** (COM port where Proffie is connected)

### Verify/Compile

1. Click the **Verify** button (✓ checkmark icon) to compile
2. Wait for compilation to complete
3. Check the output window for any errors

**Expected output:**
```
Sketch uses XXXXX bytes (XX%) of program storage space.
Global variables use XXXXX bytes (XX%) of dynamic memory.
```

### Upload (Optional)

1. Connect Proffie V3.9 via USB
2. Click the **Upload** button (→ arrow icon)
3. Wait for upload to complete

---

## Part 4: Build ESP32 Firmware

### Open the Project

1. Navigate to:
   ```
   \\wsl$\Ubuntu\mnt\c\Windows\system32\torch-project\esp32_firmware
   ```

2. Double-click `esp32_firmware.ino` to open in Arduino IDE

### Configure Board Settings

1. Go to **Tools → Board** and select:
   - **ESP32 Arduino → ESP32 Dev Module**

2. Set the following options under **Tools**:
   - **Upload Speed**: 921600
   - **CPU Frequency**: 240MHz (WiFi/BT)
   - **Flash Frequency**: 80MHz
   - **Flash Mode**: QIO
   - **Flash Size**: 4MB (32Mb)
   - **Partition Scheme**: Default 4MB with spiffs
   - **Core Debug Level**: Verbose
   - **PSRAM**: Disabled

3. Select your **Port** (COM port where ESP32 is connected)

### Verify/Compile

1. Click the **Verify** button (✓ checkmark icon)
2. Wait for compilation to complete
3. Check the output window for any errors

**Expected output:**
```
Sketch uses XXXXX bytes (XX%) of program storage space.
Global variables use XXXXX bytes (XX%) of dynamic memory.
```

### Upload (Optional)

1. Connect ESP32 via USB
2. Click the **Upload** button (→ arrow icon)
3. Some ESP32 boards require holding the BOOT button during upload
4. Wait for upload to complete

---

## Troubleshooting

### Proffie Compilation Errors

**Error: `Wire.h: No such file or directory`**
- Solution: Wire library should be built-in. Try reinstalling STM32 board support.

**Error: `Adafruit_NeoPixel.h: No such file or directory`**
- Solution: Install Adafruit NeoPixel library via Library Manager.

**Error: `'I2C_SDA_PIN' was not declared in this scope`**
- Solution: Check that `config.h` is in the same folder as `proffie_firmware.ino`.

**Error: Board not found or upload fails**
- Solution: Make sure you selected "Generic L433CCTx" as the board part number.
- Check USB cable and drivers (ST-Link drivers may be needed).

### ESP32 Compilation Errors

**Error: `esp_now.h: No such file or directory`**
- Solution: Make sure ESP32 board support is installed (version 2.0.0 or later).

**Error: `WiFi.h: No such file or directory`**
- Solution: WiFi library is built-in with ESP32 board support. Reinstall if missing.

**Error: Upload fails or "Connecting..." timeout**
- Solution: 
  - Hold the BOOT button on ESP32 while clicking Upload
  - Try a different USB cable (data cable, not charge-only)
  - Reduce upload speed to 115200 in Tools menu

### General Issues

**Error: Multiple files with same name**
- Solution: Make sure `.cpp` and `.h` files are in the same folder as the `.ino` file.

**Error: `fatal error: ../common/uart_protocol.h: No such file or directory`**
- Solution: Arduino IDE doesn't support relative paths outside the sketch folder. Copy `common/uart_protocol.h` into both `proffie_firmware/` and `esp32_firmware/` folders, or update the include path to `"uart_protocol.h"` and place the file in each firmware folder.

---

## Build Output Location

After successful compilation, the compiled binaries are stored in:

**Windows:**
```
C:\Users\<YourUsername>\AppData\Local\Temp\arduino\sketches\<sketch_hash>\
```

**The `.bin` or `.elf` files can be used for manual flashing with external tools.**

---

## Next Steps

After successful compilation:

1. **Test on hardware**: Upload to both Proffie and ESP32 boards
2. **Verify serial output**: Open Serial Monitor (115200 baud) to see debug messages
3. **Check wiring**: Refer to `docs/wiring.md` for connection details
4. **Test motion capture**: Move the Proffie board and observe LED/audio feedback
5. **Test transmission**: Shake the device to trigger ESP-NOW transmission

---

## Quick Reference: File Structure

```
proffie_firmware/
├── proffie_firmware.ino    ← Open this in Arduino IDE
├── config.h
├── lsm6ds3_driver.h/cpp
├── shake_detector.h/cpp
├── motion_buffer.h/cpp
├── led_patterns.h/cpp
├── audio_tones.h/cpp
└── uart_comm.h/cpp

esp32_firmware/
├── esp32_firmware.ino      ← Open this in Arduino IDE
├── uart_handler.h/cpp
└── espnow_handler.h/cpp
```

**Important**: All `.h` and `.cpp` files must be in the same folder as the `.ino` file for Arduino IDE to find them.
