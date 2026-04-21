# Quick Start Guide: Motion Capture & Playback System

## Overview
This system allows two devices to capture motion patterns and transmit them wirelessly to each other. Each device consists of a Proffie V3.9 board (with onboard LSM6DS3 IMU) paired with an ESP32 module for wireless communication.

---

## Hardware Requirements

### Per Device (you need 2 complete devices):
- 1× Proffie V3.9 board (with onboard LSM6DS3 IMU)
- 1× ESP32 development board (ESP32-WROOM or similar)
- 1× RGBW LED strip (WS2812-compatible with RGBW support)
- 1× Speaker (8Ω, 0.5-1W recommended)
- 1× 5V power supply (2A minimum recommended)
- Jumper wires for connections

---

## Wiring

### Device 1 & Device 2 (identical wiring for both):

#### Proffie V3.9 ↔ ESP32 UART Connection:
```
Proffie Pin 0 (TX) → ESP32 GPIO16 (RX)
Proffie Pin 1 (RX) → ESP32 GPIO17 (TX)
Proffie GND        → ESP32 GND
```

#### RGBW LED Strip → Proffie:
```
LED Strip +5V   → Proffie 5V (or external 5V supply)
LED Strip Data  → Proffie Pin 2
LED Strip GND   → Proffie GND
```

#### Speaker → Proffie:
```
Speaker (+) → Proffie DAC/Audio Out (Pin 5)
Speaker (-) → Proffie GND
```

#### Power:
```
5V Supply (+) → Proffie 5V, ESP32 5V (VIN), LED Strip 5V
5V Supply (-) → Common GND (connect all grounds together)
```

**Important Notes:**
- All grounds must be connected together (common ground)
- If using long LED strips (>30 LEDs), use external 5V power supply
- Total current budget: ~2A (Proffie: 200mA, ESP32: 200mA, LEDs: ~60mA/LED, Speaker: 500mA peak)

See `docs/wiring.md` for detailed pinout and power budget information.

---

## Software Setup

### 1. Install Required Tools

#### For Proffie (Arduino IDE):
1. Download and install [Arduino IDE](https://www.arduino.cc/en/software) (1.8.19 or 2.x)
2. Install Proffie board support:
   - Open Arduino IDE
   - Go to **File → Preferences**
   - Add to "Additional Board Manager URLs": 
     ```
     https://proffieboard.com/package_proffieboard_index.json
     ```
   - Go to **Tools → Board → Boards Manager**
   - Search for "Proffieboard" and install it
3. Install required libraries:
   - Go to **Sketch → Include Library → Manage Libraries**
   - Search and install:
     - **Adafruit NeoPixel** (for RGBW LED control)
     - **Wire** (should be built-in)

#### For ESP32 (PlatformIO - Recommended):
1. Download and install [Visual Studio Code](https://code.visualstudio.com/)
2. Install PlatformIO extension:
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X)
   - Search for "PlatformIO IDE" and install
3. The `platformio.ini` file is already configured in `esp32_firmware/`

**Alternative (Arduino IDE for ESP32):**
1. In Arduino IDE, go to **File → Preferences**
2. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools → Board → Boards Manager**
4. Search for "esp32" and install "esp32 by Espressif Systems"

---

### 2. Upload Firmware

#### Upload to Proffie V3.9:
1. Open `proffie_firmware/proffie_firmware.ino` in Arduino IDE
2. Select **Tools → Board → Proffieboard V3**
3. Select the correct **Port** (COM port where Proffie is connected)
4. Click **Upload** (→ button)
5. Wait for "Done uploading" message
6. Repeat for the second Proffie board

#### Upload to ESP32 (PlatformIO):
1. Open VS Code
2. Open the `torch-project` folder
3. Open PlatformIO sidebar (alien icon)
4. Connect ESP32 via USB
5. Click **Upload** under `esp32_firmware` environment
6. Wait for upload to complete
7. Repeat for the second ESP32 module

#### Upload to ESP32 (Arduino IDE):
1. Open `esp32_firmware/esp32_firmware.ino` in Arduino IDE
2. Select **Tools → Board → ESP32 Dev Module**
3. Select the correct **Port**
4. Click **Upload**
5. Repeat for the second ESP32

---

## Operation

### System States

Each device operates in one of these states:

1. **IDLE** (default on power-up)
   - LED: Slow breathing pattern (blue)
   - Waiting for motion

2. **RECORDING** (triggered by movement)
   - LED: Real-time motion-mapped colors
   - Audio: Real-time motion-mapped tones
   - Duration: 10 seconds
   - Automatically stops after 10 seconds

3. **TRANSMITTING** (triggered by shake gesture)
   - LED: Fast pulsing (green)
   - Sends recorded sequence to other device via ESP-NOW
   - Returns to IDLE when complete

4. **RECEIVING** (automatic when other device transmits)
   - LED: Solid (cyan)
   - Receives sequence from other device
   - Automatically transitions to PLAYBACK

5. **PLAYBACK** (automatic after receiving)
   - LED: Plays back received light pattern
   - Audio: Plays back received sound pattern
   - Returns to IDLE when complete

---

### How to Use

#### First-Time Setup:
1. Power on both devices
2. Wait for both to enter IDLE state (slow blue breathing LED)
3. Devices are now ready

#### Recording and Transmitting a Pattern:

**On Device 1:**
1. Move the device around for up to 10 seconds
   - LEDs will show real-time colors based on motion
   - Speaker will play tones based on motion intensity
2. Recording stops automatically after 10 seconds
3. **Shake the device quickly** (fast back-and-forth motion)
   - This triggers transmission
   - LED will pulse green rapidly
4. Device 1 returns to IDLE

**On Device 2 (automatically):**
1. Receives the pattern (LED turns solid cyan)
2. Plays back the same light and sound sequence
3. Returns to IDLE when playback completes

#### Sending Pattern Back:

**On Device 2:**
1. Move the device to record a new pattern (or use the one just received)
2. **Shake the device** to transmit back to Device 1
3. Device 1 will receive and play back

---

## Troubleshooting

### LEDs not lighting up:
- Check LED strip data pin connection (Proffie Pin 2)
- Verify LED strip is RGBW type (not RGB)
- Check 5V power supply to LED strip
- Ensure common ground connection

### No audio output:
- Check speaker connection (Proffie Pin 5 and GND)
- Verify speaker impedance (8Ω recommended)
- Check volume (may be low on first boot)

### ESP32 not communicating:
- Verify UART wiring (TX→RX, RX→TX, GND→GND)
- Check baud rate is 115200 on both devices
- Ensure ESP32 is powered (LED should be on)
- Check Serial Monitor on ESP32 for debug messages

### Shake detection not working:
- Shake must be fast and deliberate (>2.5g acceleration)
- Try a quick back-and-forth motion
- Check Serial Monitor on Proffie for "Shake detected!" message

### Devices not pairing via ESP-NOW:
- ESP-NOW uses broadcast mode (no pairing required)
- Both ESP32 modules must be on same WiFi channel (default: 1)
- Check ESP32 Serial Monitor for "ESP-NOW Init Success"
- Ensure both devices are powered on

### Serial Monitor shows errors:
- **Proffie**: Open Arduino IDE → Tools → Serial Monitor (115200 baud)
- **ESP32**: Open PlatformIO → Serial Monitor or Arduino IDE Serial Monitor (115200 baud)
- Look for error messages and check connections

---

## Testing Checklist

- [ ] Both Proffie boards power on and show slow blue breathing LED
- [ ] Both ESP32 modules power on (onboard LED lit)
- [ ] Moving Device 1 triggers LED color changes
- [ ] Moving Device 1 triggers audio tones
- [ ] Shaking Device 1 triggers green pulsing LED
- [ ] Device 2 receives transmission (cyan LED)
- [ ] Device 2 plays back the pattern
- [ ] Reverse transmission works (Device 2 → Device 1)

---

## LED Color Reference

### Motion Recording (real-time):
- **Blue**: Low motion intensity
- **Green**: Medium-low motion
- **Yellow**: Medium-high motion
- **Red**: High motion intensity

### System States:
- **Slow blue breathing**: IDLE (waiting)
- **Real-time colors**: RECORDING
- **Fast green pulsing**: TRANSMITTING
- **Solid cyan**: RECEIVING
- **Playback colors**: PLAYBACK (replays recorded pattern)

---

## Audio Tone Reference

- **Low pitch (200-600 Hz)**: Low motion intensity
- **Medium pitch (600-1400 Hz)**: Medium motion
- **High pitch (1400-2000 Hz)**: High motion intensity

Tones are generated in real-time during recording and replayed during playback.

---

## Power Consumption

Typical current draw per device:
- Proffie V3.9: ~200 mA
- ESP32: ~200 mA (active), ~80 mA (idle)
- RGBW LED strip: ~60 mA per LED (at full white)
- Speaker: ~100-500 mA (peak during audio)

**Recommended power supply**: 5V @ 2A per device (for up to 20 LEDs)

For longer LED strips, use a dedicated 5V power supply rated for the total LED count.

---

## Next Steps

- Experiment with different motion patterns
- Adjust shake detection sensitivity in `shake_detector.cpp` (line 45: `SHAKE_THRESHOLD`)
- Customize LED color mapping in `led_patterns.cpp` (line 25-40)
- Adjust audio frequency range in `audio_tones.cpp` (line 20-21)
- Extend recording duration in `motion_buffer.h` (line 8: `MOTION_BUFFER_SIZE`)

---

## Support

For detailed technical information, see:
- `docs/wiring.md` — Complete wiring diagrams and pinouts
- `common/uart_protocol.h` — Communication protocol specification
- `plan-motion-capture-playback-system.md` — Full implementation plan

Happy building! 🎉
