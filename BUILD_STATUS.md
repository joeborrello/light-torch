# Build Status - Torch Project

## ✅ Ready for Arduino IDE Build

Both firmware projects are now configured for Arduino IDE compilation on Windows.

---

## Changes Made for Arduino IDE Compatibility

### 1. **Fixed Include Paths**
   - Copied `common/uart_protocol.h` into both firmware directories
   - Updated includes from `"../common/uart_protocol.h"` to `"uart_protocol.h"`
   - Arduino IDE requires all files in the same folder as the `.ino` file

### 2. **Added Missing Packet Type Definitions**
   - Added `PKT_*` constants to `esp32_firmware/uart_protocol.h`
   - These map to the `MessageType` enum values

---

## Build Instructions

### Quick Start

1. **Open Arduino IDE** (on Windows, not WSL)

2. **Build Proffie Firmware:**
   - Open: `proffie_firmware/proffie_firmware.ino`
   - Board: STM32 → Generic STM32L4 series → Generic L433CCTx
   - Click **Verify** (✓) to compile

3. **Build ESP32 Firmware:**
   - Open: `esp32_firmware/esp32_firmware.ino`
   - Board: ESP32 Arduino → ESP32 Dev Module
   - Click **Verify** (✓) to compile

### Detailed Instructions

See **`docs/ARDUINO_BUILD_GUIDE.md`** for:
- Board support installation
- Library installation (Adafruit_NeoPixel)
- Complete board settings
- Troubleshooting guide

---

## File Structure (Arduino IDE Compatible)

```
proffie_firmware/
├── proffie_firmware.ino    ← Main sketch
├── config.h
├── uart_protocol.h         ← Copied from common/
├── lsm6ds3_driver.h
├── lsm6ds3_driver.cpp
├── shake_detector.h
├── shake_detector.cpp
├── motion_buffer.h
├── motion_buffer.cpp
├── led_patterns.h
├── led_patterns.cpp
├── audio_tones.h
├── audio_tones.cpp
├── uart_comm.h
└── uart_comm.cpp

esp32_firmware/
├── esp32_firmware.ino      ← Main sketch
├── uart_protocol.h         ← Copied from common/ + PKT_ defines
├── uart_handler.h
├── uart_handler.cpp
├── espnow_handler.h
└── espnow_handler.cpp
```

---

## Expected Compilation Results

### Proffie (STM32L433CC)
- **Flash**: ~40-60KB (out of 256KB)
- **RAM**: ~10-15KB (out of 64KB)
- **Compile time**: 30-60 seconds

### ESP32
- **Flash**: ~200-300KB (out of 4MB)
- **RAM**: ~30-40KB (out of 520KB)
- **Compile time**: 60-120 seconds

---

## Known Issues / Notes

### Arduino IDE Limitations
- Cannot use relative paths outside sketch folder (`../common/`)
- All `.h` and `.cpp` files must be in same folder as `.ino`
- Solution: Duplicated `uart_protocol.h` in both firmware folders

### WSL vs Windows
- Arduino IDE should be run on **Windows** (not WSL)
- Access project files via: `\\wsl$\Ubuntu\mnt\c\Windows\system32\torch-project\`
- COM ports are only accessible from Windows, not WSL

### Library Dependencies
- **Proffie**: Adafruit_NeoPixel (install via Library Manager)
- **ESP32**: WiFi, esp_now (built-in with ESP32 board support)

---

## Next Steps After Successful Build

1. **Upload to hardware**
   - Connect Proffie via USB → Upload
   - Connect ESP32 via USB → Upload

2. **Test serial output**
   - Open Serial Monitor (115200 baud)
   - Verify initialization messages

3. **Test motion capture**
   - Move Proffie board
   - Observe LED patterns and audio tones

4. **Test wireless transmission**
   - Shake device to trigger transmission
   - Check ESP32 serial output for ESP-NOW activity

5. **Verify playback**
   - Second device should receive and play back motion

---

## Troubleshooting

If compilation fails, check:

1. **Board support installed?**
   - STM32 boards (for Proffie)
   - ESP32 boards

2. **Libraries installed?**
   - Adafruit_NeoPixel

3. **Correct board selected?**
   - Proffie: Generic L433CCTx
   - ESP32: ESP32 Dev Module

4. **All files in same folder?**
   - `.ino`, `.h`, and `.cpp` files together

See `docs/ARDUINO_BUILD_GUIDE.md` for detailed troubleshooting.

---

## Build System Summary

| Component | Build System | IDE | Platform |
|-----------|--------------|-----|----------|
| Proffie Firmware | Arduino | Arduino IDE | Windows |
| ESP32 Firmware | Arduino | Arduino IDE | Windows |
| Original ESP32 | PlatformIO | VS Code | Windows/WSL |

**Note**: PlatformIO build is still available but requires installation. Arduino IDE is simpler for Windows + WSL setup.
