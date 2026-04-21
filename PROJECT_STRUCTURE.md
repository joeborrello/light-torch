# Project Structure

```
torch-project/
│
├── proffie_firmware/              # Proffie V3.9 firmware (Arduino)
│   ├── proffie_firmware.ino       # Main state machine and loop
│   ├── config.h                   # Configuration constants
│   ├── lsm6ds3_driver.h/cpp       # I2C driver for LSM6DS3 IMU
│   ├── shake_detector.h/cpp       # Shake detection algorithm
│   ├── motion_buffer.h/cpp        # Circular buffer for motion samples
│   ├── led_patterns.h/cpp         # RGBW LED pattern generator
│   ├── audio_tones.h/cpp          # Audio tone synthesis
│   └── uart_comm.h/cpp            # UART communication with ESP32
│
├── esp32_firmware/                # ESP32 firmware (PlatformIO)
│   ├── esp32_firmware.ino         # Main loop and state machine
│   ├── platformio.ini             # PlatformIO configuration
│   ├── uart_handler.h/cpp         # UART communication with Proffie
│   └── espnow_handler.h/cpp       # ESP-NOW wireless communication
│
├── common/                        # Shared protocol definitions
│   └── uart_protocol.h            # Packet structures for UART
│
├── docs/                          # Documentation
│   └── wiring.md                  # Wiring diagrams and pinouts
│
├── plan-motion-capture-playback-system.md  # Implementation plan (COMPLETE)
├── SEB.MD                         # Project context and configuration
├── README.md                      # Full system documentation
└── QUICKSTART.md                  # 30-minute setup guide
```

## File Count Summary

- **Proffie firmware**: 15 files (7 headers, 7 implementations, 1 main sketch)
- **ESP32 firmware**: 6 files (2 headers, 2 implementations, 1 main sketch, 1 config)
- **Shared protocol**: 1 file
- **Documentation**: 5 files
- **Total**: 27 files

## Lines of Code (Estimated)

| Component | Files | Lines |
|-----------|-------|-------|
| Proffie firmware | 15 | ~1,200 |
| ESP32 firmware | 6 | ~600 |
| Documentation | 5 | ~800 |
| **Total** | **26** | **~2,600** |

## Key Features Implemented

### ✅ Phase 1: LSM6DS3 Driver and Shake Detection
- I2C communication at 400kHz
- Accelerometer and gyroscope reading (±4g, ±500dps)
- Shake detection with 2.5g threshold and 1-second cooldown

### ✅ Phase 2: Motion Capture and Pattern Generation
- 10-second circular buffer (500 samples @ 50Hz)
- RGBW LED patterns mapped to motion intensity
- Audio tone generation (200-2000Hz) mapped to motion
- UART protocol definition with packet framing and checksums

### ✅ Phase 3: UART Communication
- Proffie-side UART driver with ACK/NACK
- Packet validation and error handling
- START_TRANSMIT, MOTION_DATA, END_TRANSMIT message types

### ✅ Phase 4: ESP32 Firmware
- UART bridge between Proffie and ESP-NOW
- ESP-NOW broadcast mode for peer-to-peer communication
- State machine (idle, receiving from Proffie, transmitting, receiving from peer, forwarding to Proffie)
- Status LED indicators

### ✅ Phase 5: Integration
- Complete Proffie state machine (IDLE → RECORDING → TRANSMITTING → RECEIVING → PLAYBACK)
- Centralized configuration in `config.h`
- Serial debug output for monitoring
- Automatic role swapping after playback

## Build Instructions

### Proffie Firmware
```bash
# Open in Arduino IDE
# Select: Tools → Board → STM32L4 → Generic STM32L4 series
# Select: Tools → Board part number → STM32L433CC
# Select: Tools → Upload method → STLink
# Click Upload
```

### ESP32 Firmware
```bash
cd esp32_firmware
pio run -t upload
pio device monitor  # Get MAC address
```

## Configuration Checklist

- [ ] Update `PEER_MAC` in `proffie_firmware/config.h` with peer ESP32 MAC
- [ ] Set `NUM_LEDS` to match your LED strip length
- [ ] Adjust `LED_BRIGHTNESS` if needed (0-255)
- [ ] Verify `IMU_I2C_SDA` and `IMU_I2C_SCL` pins (14, 13)
- [ ] Confirm `LED_DATA_PIN` matches your wiring (default: 5)

## Testing Checklist

- [ ] Proffie powers on and initializes LSM6DS3
- [ ] LEDs respond to motion in real-time
- [ ] Audio tones change with motion intensity
- [ ] Shake detection triggers transmission
- [ ] ESP32 receives data from Proffie via UART
- [ ] ESP-NOW transmission succeeds
- [ ] Peer device receives and plays back motion
- [ ] Role swap works (receiver becomes transmitter)

## Power Budget

| Component | Current (mA) | Notes |
|-----------|--------------|-------|
| Proffie V3.9 | ~50 | Idle |
| ESP32 | ~80 | WiFi active |
| LSM6DS3 | ~1 | Normal mode |
| LED strip (60 LEDs @ 50%) | ~1,500 | Worst case |
| Speaker | ~100 | Peak |
| **Total** | **~1,730** | **Use 2-3A supply** |

## Next Development Steps

1. **Optimize power consumption**: Add sleep modes when idle
2. **Add SD card logging**: Record motion data for later analysis
3. **Implement pattern library**: Save/load multiple motion sequences
4. **Add OLED display**: Show status, battery level, menu
5. **Multi-device support**: Broadcast to >2 devices simultaneously
6. **Gesture recognition**: Detect specific motion patterns (swipe, circle, etc.)
7. **Battery monitoring**: ADC reading of battery voltage with low-battery warning

## Known Limitations

- **Recording duration**: Fixed at 10 seconds (configurable in `config.h`)
- **LED count**: Maximum ~100 LEDs due to power and timing constraints
- **Range**: ESP-NOW limited to ~50m line-of-sight
- **Timing precision**: ±20ms due to 50Hz sampling rate
- **Memory**: Motion buffer uses ~8KB RAM (limits other features)

## License

MIT License — See LICENSE file for details
