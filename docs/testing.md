# Testing Guide

## Phase 1: Individual Component Testing

### Test 1: LSM6DS3 IMU Communication

**Objective**: Verify I2C communication with onboard IMU

**Steps**:
1. Upload `proffie_firmware.ino` to Proffie
2. Open Serial Monitor (115200 baud)
3. Look for: `LSM6DS3 initialized`
4. Move the device and observe IMU data in serial output

**Expected Result**:
- No "LSM6DS3 init failed!" error
- Accelerometer and gyroscope values change with motion

**Troubleshooting**:
- If init fails, check I2C pins (SDA=14, SCL=13)
- Verify I2C address 0x6A (use I2C scanner sketch if needed)

---

### Test 2: Shake Detection

**Objective**: Verify shake gesture recognition

**Steps**:
1. With Proffie running, shake the device rapidly
2. Observe Serial Monitor

**Expected Result**:
- Message: `Shake detected! State: TRANSMITTING`
- Should trigger only on fast shakes (>2.5g)
- 1-second cooldown between detections

**Troubleshooting**:
- If too sensitive: increase `SHAKE_THRESHOLD_G` in `config.h`
- If not sensitive enough: decrease threshold

---

### Test 3: LED Patterns

**Objective**: Verify RGBW LED strip control

**Steps**:
1. Connect LED strip to Proffie (Data1 = pin 0)
2. Power on system
3. Move device and observe LED colors

**Expected Result**:
- LEDs light up in recording mode (pulsing green)
- Colors change with motion intensity:
  - Low motion: Blue
  - Medium motion: Green/Yellow
  - High motion: Red

**Troubleshooting**:
- No LEDs: Check 5V power, data pin connection, common ground
- Wrong colors: Verify LED type (RGBW vs RGB) in code
- Flickering: Check power supply capacity (60 LEDs @ full white = ~3.6A)

---

### Test 4: Audio Tones

**Objective**: Verify speaker output

**Steps**:
1. Connect speaker to Proffie speaker terminals
2. Move device with varying speeds

**Expected Result**:
- Tone pitch increases with motion intensity
- Frequency range: 200-2000 Hz
- Smooth transitions

**Troubleshooting**:
- No sound: Check speaker polarity, verify DAC initialization
- Distorted sound: Check speaker impedance (4-8Ω recommended)

---

## Phase 2: UART Communication Testing

### Test 5: Proffie → ESP32 UART

**Objective**: Verify UART data transmission

**Steps**:
1. Connect Proffie TX → ESP32 RX, RX → TX, GND → GND
2. Upload ESP32 firmware
3. Open two Serial Monitors:
   - Proffie: 115200 baud
   - ESP32: 115200 baud
4. Trigger shake on Proffie

**Expected Result**:
- Proffie: `Transmitting 500 samples...`
- ESP32: `Received motion sample [0]`, `[1]`, etc.
- Proffie: `Transmission complete`

**Troubleshooting**:
- No data: Check TX/RX crossover, verify baud rate
- Checksum errors: Check wiring quality, add ground connection
- Timeouts: Increase `UART_TIMEOUT_MS` in config

---

## Phase 3: ESP-NOW Wireless Testing

### Test 6: ESP32 ↔ ESP32 ESP-NOW

**Objective**: Verify wireless transmission between devices

**Setup**:
- Device A: Proffie + ESP32 (transmitter)
- Device B: Proffie + ESP32 (receiver)
- Both ESP32s powered and running firmware

**Steps**:
1. Power on both devices
2. Trigger shake on Device A
3. Observe Serial Monitors on both ESP32s

**Expected Result**:
- Device A ESP32: `Transmitting via ESP-NOW...`
- Device B ESP32: `Received ESP-NOW packet`
- Device B Proffie: `Incoming data detected. State: PLAYBACK`

**Troubleshooting**:
- No reception: Check WiFi channel (both must be on channel 1)
- Packet loss: Reduce distance, check for WiFi interference
- Pairing issues: Verify broadcast MAC (FF:FF:FF:FF:FF:FF)

---

## Phase 4: End-to-End System Testing

### Test 7: Full Motion Capture and Playback

**Objective**: Complete workflow from capture to playback

**Steps**:
1. Power on Device A and Device B
2. Move Device A in a pattern (e.g., figure-8, circle)
3. Observe LEDs and audio responding in real-time
4. Shake Device A to transmit
5. Observe Device B playing back the pattern
6. Verify Device B enters recording mode after playback

**Expected Result**:
- Device A records 10 seconds of motion
- LEDs and audio match motion in real-time
- Shake triggers transmission
- Device B receives and plays back identical pattern
- Device B ready to record and transmit back

**Success Criteria**:
- ✅ LED patterns match on both devices during playback
- ✅ Audio tones match on both devices
- ✅ Timing is synchronized (50Hz playback)
- ✅ No data corruption or packet loss
- ✅ State transitions work correctly

---

## Phase 5: Stress Testing

### Test 8: Rapid Transmission Cycles

**Objective**: Test system stability under repeated use

**Steps**:
1. Perform 10 consecutive transmit/receive cycles
2. Alternate between Device A and Device B
3. Monitor for memory leaks, crashes, or degradation

**Expected Result**:
- All 10 cycles complete successfully
- No memory errors or buffer overflows
- Consistent performance across cycles

---

### Test 9: Maximum Motion Intensity

**Objective**: Test system limits

**Steps**:
1. Record very fast, aggressive motion
2. Verify buffer doesn't overflow
3. Check LED and audio stay within bounds

**Expected Result**:
- No crashes or hangs
- LED brightness capped at `LED_BRIGHTNESS`
- Audio frequency capped at 2000 Hz
- Motion buffer handles 500 samples without overflow

---

## Debugging Tools

### Serial Monitor Commands

Add these to `proffie_firmware.ino` for debugging:

```cpp
void loop() {
  // ... existing code ...
  
  // Debug commands via Serial
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'd': // Dump buffer
        Serial.print("Buffer samples: ");
        Serial.println(motionBuffer.getCount());
        break;
      case 's': // Force shake
        Serial.println("Forcing shake detection");
        state = TRANSMITTING;
        break;
      case 'r': // Reset to recording
        state = RECORDING;
        motionBuffer.reset();
        break;
    }
  }
}
```

### ESP32 Debug Output

Enable verbose ESP-NOW logging in `espnow_handler.cpp`:

```cpp
void ESPNowHandler::onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
  Serial.print("ESP-NOW RX from: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.printf(" | Length: %d bytes\n", len);
  // ... rest of handler ...
}
```

---

## Performance Benchmarks

**Expected Performance**:
- IMU sample rate: 50 Hz ±2 Hz
- UART throughput: ~25 KB/s (500 samples in ~2 seconds)
- ESP-NOW latency: <100ms for full transmission
- LED update rate: 50 FPS
- Audio latency: <10ms

**Measure with**:
```cpp
uint32_t start = micros();
// ... operation ...
uint32_t elapsed = micros() - start;
Serial.print("Operation took: ");
Serial.print(elapsed);
Serial.println(" us");
```

---

## Common Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| IMU init fails | I2C wiring or address | Check SDA/SCL pins, verify 0x6A address |
| LEDs flicker | Insufficient power | Use external 5V supply for LED strip |
| UART timeouts | Baud mismatch | Verify both sides use 115200 baud |
| ESP-NOW fails | WiFi channel mismatch | Both ESP32s must be on same channel |
| Audio distortion | Speaker impedance | Use 4-8Ω speaker, check DAC config |
| Memory errors | Buffer overflow | Reduce `RECORDING_DURATION_SEC` |
| Shake too sensitive | Low threshold | Increase `SHAKE_THRESHOLD_G` |
| Playback out of sync | Timing drift | Verify 50Hz playback rate (20ms delay) |

---

## Next Steps After Testing

Once all tests pass:
1. Calibrate shake threshold for your use case
2. Tune LED brightness and color mapping
3. Adjust audio frequency range
4. Optimize power consumption (sleep modes)
5. Add battery monitoring
6. Implement user feedback (button, status LED)
