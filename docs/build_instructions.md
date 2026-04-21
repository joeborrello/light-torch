# Build and Flash Instructions

## Prerequisites

### For Proffie V3.9 Firmware

1. **Arduino IDE** (1.8.19 or 2.x)
   - Download from: https://www.arduino.cc/en/software

2. **STM32duino Board Support**
   - In Arduino IDE: `File` → `Preferences` → `Additional Board Manager URLs`
   - Add: `https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json`
   - Go to `Tools` → `Board` → `Boards Manager`
   - Search for "STM32" and install "STM32 MCU based boards" by STMicroelectronics

3. **Required Libraries**
   - **Adafruit NeoPixel**: `Sketch` → `Include Library` → `Manage Libraries` → Search "Adafruit NeoPixel" → Install
   - **Wire** (built-in, no install needed)

### For ESP32 Firmware

**Option A: PlatformIO (Recommended)**
1. Install **Visual Studio Code**: https://code.visualstudio.com/
2. Install **PlatformIO IDE** extension in VS Code
3. No additional setup needed — `platformio.ini` handles dependencies

**Option B: Arduino IDE**
1. In Arduino IDE: `File` → `Preferences` → `Additional Board Manager URLs`
2. Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Go to `Tools` → `Board` → `Boards Manager`
4. Search for "ESP32" and install "esp32" by Espressif Systems

---

## Building Proffie Firmware

### Using Arduino IDE

1. **Open the project**:
   ```
   File → Open → proffie_firmware/proffie_firmware.ino
   ```

2. **Configure board settings**:
   - `Tools` → `Board` → `STM32 boards groups` → `Generic STM32F4 series`
   - `Tools` → `Board part number` → `Generic F411CEUx`
   - `Tools` → `Upload method` → `STM32CubeProgrammer (DFU)` or `Serial` (depending on your setup)
   - `Tools` → `USB support` → `CDC (generic 'Serial' supersede U(S)ART)`
   - `Tools` → `U(S)ART support` → `Enabled (generic 'Serial')`

3. **Verify/Compile**:
   ```
   Sketch → Verify/Compile
   ```
   - Check for errors in the console
   - Should see "Done compiling" if successful

4. **Upload to Proffie**:
   - Connect Proffie V3.9 via USB
   - `Sketch` → `Upload`
   - Wait for "Done uploading"

---

## Building ESP32 Firmware

### Using PlatformIO (Recommended)

1. **Open the project in VS Code**:
   ```
   File → Open Folder → esp32_firmware/
   ```

2. **Build the firmware**:
   - Click the **checkmark icon** (✓) in the PlatformIO toolbar
   - Or open terminal: `pio run`

3. **Upload to ESP32**:
   - Connect ESP32 via USB
   - Click the **arrow icon** (→) in the PlatformIO toolbar
   - Or open terminal: `pio run --target upload`

4. **Monitor serial output**:
   - Click the **plug icon** in the PlatformIO toolbar
   - Or open terminal: `pio device monitor`

### Using Arduino IDE

1. **Open the project**:
   ```
   File → Open → esp32_firmware/esp32_firmware.ino
   ```

2. **Configure board settings**:
   - `Tools` → `Board` → `ESP32 Arduino` → `ESP32 Dev Module`
   - `Tools` → `Upload Speed` → `115200`
   - `Tools` → `Flash Frequency` → `80MHz`
   - `Tools` → `Partition Scheme` → `Default 4MB with spiffs`

3. **Verify/Compile**:
   ```
   Sketch → Verify/Compile
   ```

4. **Upload to ESP32**:
   - Connect ESP32 via USB
   - `Tools` → `Port` → Select your ESP32's COM port
   - `Sketch` → `Upload`

---

## Post-Flash Configuration

### 1. Get ESP32 MAC Addresses

After flashing both ESP32 modules, you need to find their MAC addresses:

1. Connect **ESP32 #1** via USB
2. Open Serial Monitor (115200 baud)
3. Press the **RESET** button on the ESP32
4. Note the MAC address printed (e.g., `24:6F:28:AB:CD:EF`)
5. Repeat for **ESP32 #2**

### 2. Update Peer MAC Addresses

Edit `proffie_firmware/config.h`:

```cpp
// Replace with the MAC address of the OTHER device's ESP32
#define PEER_MAC {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF}
```

**Important**: Each device should have the MAC address of its **peer**, not itself.

- **Device 1**: Set `PEER_MAC` to Device 2's ESP32 MAC
- **Device 2**: Set `PEER_MAC` to Device 1's ESP32 MAC

### 3. Re-flash Proffie Firmware

After updating `config.h`, re-upload the Proffie firmware to both devices.

---

## Verification

### Proffie Serial Output (115200 baud)
```
LSM6DS3 init OK
Proffie Motion Capture System Ready
State: IDLE -> RECORDING
```

### ESP32 Serial Output (115200 baud)
```
ESP32 Motion Bridge Ready
MAC Address: 24:6F:28:AB:CD:EF
ESP-NOW Init Success
State: IDLE
```

---

## Troubleshooting

### Proffie Upload Fails
- **DFU mode**: Hold BOOT button, press RESET, release BOOT
- **Serial mode**: Ensure USB cable supports data (not charge-only)
- **Driver issues**: Install STM32 USB drivers from ST website

### ESP32 Upload Fails
- **Hold BOOT button** during upload (some boards auto-reset, some don't)
- **Check COM port**: `Tools` → `Port` → Select correct port
- **Driver issues**: Install CP2102 or CH340 USB drivers (depends on your ESP32 board)

### Compilation Errors
- **Missing libraries**: Install Adafruit NeoPixel via Library Manager
- **Board not found**: Re-install board support packages
- **Syntax errors**: Ensure all files are in correct directories

### Runtime Issues
- **IMU not detected**: Check I2C connections (SDA=14, SCL=13)
- **LEDs not working**: Verify `NUM_LEDS` in `config.h` matches your strip
- **No ESP-NOW communication**: Verify MAC addresses are correct and devices are within range

---

## Next Steps

After successful build and flash:
1. Proceed to `wiring.md` for hardware connections
2. Test basic functionality (IMU, LEDs, audio)
3. Test wireless transmission between devices
