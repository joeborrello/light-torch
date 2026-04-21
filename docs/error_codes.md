# Error Indication Reference

This document describes all error states and their visual/audio indicators for the Motion Capture & Playback System.

## Error Codes

### 1. IMU Initialization Failure
**Cause:** LSM6DS3 sensor not detected on I2C bus  
**LED Pattern:** Solid RED (all LEDs)  
**Audio Pattern:** Low continuous buzz (200Hz, 1 second, repeating every 2 seconds)  
**Behavior:** System halts in infinite loop — cannot proceed without IMU  
**Troubleshooting:**
- Check I2C wiring (SDA/SCL connections)
- Verify Proffie board has power
- Confirm LSM6DS3 is properly soldered/seated
- Check I2C pull-up resistors (typically 4.7kΩ to 3.3V)

---

### 2. UART Timeout
**Cause:** ESP32 not responding to Proffie UART messages  
**LED Pattern:** ORANGE pulsing (breathing effect)  
**Audio Pattern:** Double beep (800Hz, 150ms each, 100ms gap)  
**Behavior:** Transmission aborts after 3 consecutive timeouts  
**Troubleshooting:**
- Check UART wiring (TX ↔ RX crossover, common GND)
- Verify ESP32 is powered and running
- Check baud rate matches on both devices (default: 115200)
- Ensure ESP32 firmware is flashed and not stuck in bootloader
- Monitor ESP32 serial output for errors

---

### 3. Buffer Overflow
**Cause:** Motion buffer exceeded 10-second capacity (500 samples @ 50Hz)  
**LED Pattern:** YELLOW fast blink (150ms on/off)  
**Audio Pattern:** Triple beep (1000Hz, 100ms each, 80ms gap)  
**Behavior:** Oldest samples discarded (circular buffer), error shown once  
**Troubleshooting:**
- Shake device sooner to trigger transmission before buffer fills
- Reduce `BUFFER_DURATION_SEC` in `motion_buffer.h` if memory constrained
- Increase shake sensitivity (`SHAKE_THRESHOLD` in `shake_detector.h`) to trigger earlier

---

### 4. ESP-NOW Transmission Failed
**Cause:** ESP32 peer device not found or wireless transmission failed  
**LED Pattern:** MAGENTA flash (300ms on/off)  
**Audio Pattern:** Descending tone (1200Hz → 400Hz over 500ms)  
**Behavior:** Transmission fails, system returns to RECORDING state  
**Troubleshooting:**
- Verify peer ESP32 is powered on and in range (<100m line-of-sight)
- Check peer MAC address is correctly configured in `esp32_firmware.ino`
- Ensure both ESP32s are on the same WiFi channel
- Check for WiFi interference (2.4GHz congestion)
- Verify ESP-NOW is initialized successfully (check ESP32 serial output)

---

### 5. Checksum Failure
**Cause:** Data corruption during UART transmission (noise, EMI, baud rate mismatch)  
**LED Pattern:** CYAN blink (250ms on/off)  
**Audio Pattern:** Short chirp (1500Hz, 80ms)  
**Behavior:** Corrupted sample skipped, playback continues  
**Troubleshooting:**
- Shorten UART cable length
- Add ferrite beads to UART lines
- Verify baud rate matches exactly on both devices
- Check for loose connections or damaged wires
- Reduce electrical noise sources (motors, relays, switching regulators)
- Lower baud rate if errors persist (edit `UART_BAUD` in `uart_protocol.h`)

---

## Normal Operation Indicators

For comparison, here are the normal (non-error) LED patterns:

| State | LED Pattern | Description |
|-------|-------------|-------------|
| **IDLE** | Dim white breathing | System ready, waiting to start recording |
| **RECORDING** | Pulsing red | Actively capturing motion data |
| **TRANSMITTING** | Flashing blue | Sending data to ESP32 via UART |
| **RECEIVING** | Purple/magenta breathing | Receiving motion data from peer device |
| **PLAYBACK** | Motion-reactive colors | Playing back received motion (blue→green→yellow→red gradient based on intensity) |

---

## Error Recovery Strategies

### Automatic Recovery
- **Buffer Overflow:** System continues recording (circular buffer)
- **Checksum Failure:** Skips corrupted sample, continues playback
- **UART Timeout (1-2 occurrences):** Retries transmission

### Manual Recovery Required
- **IMU Failure:** Power cycle device, check hardware
- **UART Timeout (3+ consecutive):** Check ESP32 connection, power cycle both devices
- **ESP-NOW Failure:** Verify peer device is on, check MAC address configuration

### Debug Mode
To diagnose issues, connect to Proffie's USB serial port (115200 baud) and monitor debug messages:
```
=== Proffie Motion Capture System ===
LSM6DS3 initialized
LED patterns initialized
Audio initialized
UART initialized
State: RECORDING
Shake detected! State: TRANSMITTING
Transmitting 347 samples...
ACK timeout at sample 142  ← UART timeout error
Checksum error detected!   ← Data corruption
```

---

## Configuration Adjustments

If you experience frequent errors, adjust these parameters:

**Shake Sensitivity** (`shake_detector.h`):
```cpp
#define SHAKE_THRESHOLD 2.5  // Lower = more sensitive (range: 1.5-4.0g)
```

**Buffer Size** (`motion_buffer.h`):
```cpp
#define BUFFER_DURATION_SEC 10  // Reduce if RAM constrained
```

**UART Timeout** (`uart_protocol.h`):
```cpp
#define UART_TIMEOUT_MS 500  // Increase for slower ESP32 response
```

**LED Count** (`led_patterns.h`):
```cpp
#define NUM_LEDS 45  // Match your actual LED strip length
```

---

## Hardware Checklist

Before troubleshooting software, verify:

- [ ] Proffie powered from LiPo via boost converter (5V)
- [ ] ESP32 powered from same boost converter (5V) — **NOT from Proffie 3.3V regulator**
- [ ] Common ground between Proffie and ESP32
- [ ] UART TX/RX crossover: Proffie TX → ESP32 RX, Proffie RX → ESP32 TX
- [ ] LED strip data line connected to Proffie Data1 (PA7)
- [ ] Speaker connected to Proffie audio output
- [ ] I2C pull-ups present (usually on-board, but check if using external IMU)

---

## Contact & Support

If errors persist after following this guide:
1. Capture serial debug output from both Proffie and ESP32
2. Note the exact LED/audio pattern observed
3. Document when the error occurs (during transmission, playback, etc.)
4. Check GitHub issues or create a new issue with the above information
