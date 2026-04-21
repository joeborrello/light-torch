# Quick Reference Card

## LED Error Patterns (At-a-Glance)

| Color | Pattern | Error | Action |
|-------|---------|-------|--------|
| 🔴 **RED** | Solid | IMU failed | Check I2C wiring, power cycle |
| 🟠 **ORANGE** | Pulse | UART timeout | Check ESP32 connection |
| 🟡 **YELLOW** | Fast blink | Buffer overflow | Shake sooner to transmit |
| 🟣 **MAGENTA** | Flash | ESP-NOW failed | Check peer device, MAC address |
| 🔵 **CYAN** | Blink | Checksum error | Check UART wiring, reduce noise |

## Normal Operation Patterns

| Color | Pattern | State |
|-------|---------|-------|
| ⚪ **WHITE** | Breathing | Idle (ready) |
| 🔴 **RED** | Pulsing | Recording motion |
| 🔵 **BLUE** | Flashing | Transmitting to ESP32 |
| 🟣 **PURPLE** | Breathing | Receiving from peer |
| 🌈 **Rainbow** | Motion-reactive | Playing back motion |

## Audio Error Tones

| Sound | Error |
|-------|-------|
| Low buzz (200Hz, continuous) | IMU failed |
| Double beep (800Hz) | UART timeout |
| Triple beep (1000Hz) | Buffer overflow |
| Descending tone (1200→400Hz) | ESP-NOW failed |
| Short chirp (1500Hz) | Checksum error |

## User Configuration

**Change LED count:** Edit `NUM_LEDS` in `proffie_firmware/led_patterns.h`  
**Adjust shake sensitivity:** Edit `SHAKE_THRESHOLD` in `proffie_firmware/shake_detector.h`  
**Change buffer size:** Edit `BUFFER_DURATION_SEC` in `proffie_firmware/motion_buffer.h`

## Power Requirements

- **Proffie:** 5V from boost converter (NOT USB during operation)
- **ESP32:** 5V from same boost converter (**NOT from Proffie 3.3V regulator**)
- **Common ground:** Required between Proffie and ESP32

## Wiring Checklist

```
LiPo → Boost (5V) ─┬─→ Proffie VIN
                   └─→ ESP32 VIN

Proffie TX (PA9)  ──→ ESP32 RX (GPIO16)
Proffie RX (PA10) ←── ESP32 TX (GPIO17)
Proffie GND ←────────→ ESP32 GND

Proffie Data1 (PA7) ──→ LED Strip Data
Proffie Audio ────────→ Speaker
```

## Troubleshooting Flowchart

```
Error occurs
    ↓
Check LED color/pattern (see table above)
    ↓
Is it RED (solid)?
    ├─ YES → IMU hardware issue → Check I2C wiring
    └─ NO  → Continue
         ↓
Is it ORANGE (pulse)?
    ├─ YES → UART timeout → Check ESP32 power/connection
    └─ NO  → Continue
         ↓
Is it YELLOW (blink)?
    ├─ YES → Buffer overflow → Shake sooner
    └─ NO  → Continue
         ↓
Is it MAGENTA (flash)?
    ├─ YES → ESP-NOW failed → Check peer device
    └─ NO  → Continue
         ↓
Is it CYAN (blink)?
    └─ YES → Checksum error → Check UART wiring/noise
```

## First-Time Setup

1. **Flash firmware:**
   - Proffie: Use Arduino IDE (STM32L433CC board)
   - ESP32: Use PlatformIO or Arduino IDE

2. **Configure peer MAC:**
   - Flash one ESP32, note MAC from serial monitor
   - Update `PEER_MAC` in other ESP32's code
   - Reflash both

3. **Set LED count:**
   - Count LEDs in your strip
   - Edit `NUM_LEDS` in `led_patterns.h`
   - Reflash Proffie

4. **Test:**
   - Power both devices
   - Move device → LEDs/audio should respond
   - Shake sharply → should transmit (blue flash)
   - Peer device → should receive and play back

## Serial Debug Commands

Connect to Proffie USB serial (115200 baud) to see:
- Initialization status
- State transitions
- Error messages
- Sample counts during transmission

Connect to ESP32 USB serial (115200 baud) to see:
- MAC address (needed for peer config)
- ESP-NOW status
- Packet reception logs
