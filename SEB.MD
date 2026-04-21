# Project Context

## Target Hardware
- **Primary MCU/Processor**: ESP32 (Arduino Nano ESP32)
- **Secondary Processors**: STM32L433CC (Proffie V3.9 board)
- **Dev Board / Custom PCB**: Arduino Nano ESP32 dev board + Proffie V3.9 custom board
- **Board Revision**: Proffie V3.9

## Clock Configuration
- **HSE/HSI**: ESP32 internal oscillator (default)
- **PLL Config**: Managed by ESP32 Arduino framework
- **System Clock Frequency**: 240 MHz (ESP32 default)

## Pin Mapping / GPIO Assignments
- **ESP32 UART**: RX0 (GPIO 0) ← Proffie TX, TX1 (GPIO 1) → Proffie RX
- **ESP32 LED**: GPIO 13 (built-in LED, status indicator)
- **Proffie V3.9**: TX/RX pads for UART to ESP32
- **Proffie I2C**: Wire (I2C1) for LSM6DS3 IMU @ 0x6A
- **Proffie NeoPixel**: Data output to RGBW LED strip
- **Proffie Audio**: 3W amplifier output

## Memory Layout
- **Flash Size**: 4 MB (ESP32), 256 KB (STM32L433CC)
- **RAM Size**: 520 KB (ESP32), 64 KB (STM32L433CC)
- **Linker Script Regions**: Managed by PlatformIO/Arduino framework (ESP32), Arduino IDE (Proffie)

## Peripherals & Interfaces
- **ESP32**: UART (Serial1), WiFi (ESP-NOW only), GPIO
- **Proffie V3.9**: I2C1 (LSM6DS3), UART (Serial1), PWM (6 FETs for LEDs), Audio amplifier (3W)

## External Devices & Sensors
- **LSM6DS3 IMU**: 6-axis accelerometer + gyroscope @ I2C address 0x6A (Proffie V3.9 onboard)
  - Accel: ±4g @ 104 Hz
  - Gyro: ±500 dps @ 104 Hz
  - WHO_AM_I: 0x69
- **RGBW NeoPixel Strip**: 4-wire (+5V, Data, GND, White) addressable LEDs
- **Speaker**: Connected to Proffie 3W amplifier

## Communication Protocols
- **UART (Proffie ↔ ESP32)**: 115200 baud, 8N1
- **ESP-NOW (ESP32 ↔ ESP32)**: Peer-to-peer wireless, 2.4 GHz
- **I2C (Proffie ↔ LSM6DS3)**: 400 kHz (Fast Mode)

## Software Stack
- **Build System**: PlatformIO (ESP32), Arduino IDE (Proffie)
- **Compile Command**: `pio run` (ESP32), Arduino IDE build (Proffie)
- **HAL / Framework**: Arduino framework (ESP-IDF underneath for ESP32), Arduino core for STM32L4 (Proffie)
- **RTOS**: FreeRTOS (ESP32 Arduino framework), None (Proffie bare-metal)
- **Toolchain**: Xtensa GCC (ESP32), ARM GCC (STM32L433CC)
- **Compiler Flags**: `-DCORE_DEBUG_LEVEL=3` (ESP32)
- **External Libraries**: 
  - ESP32: `esp_now.h`, `WiFi.h`, `Wire`
  - Proffie: `Wire.h`, NeoPixel library (assumed)

## Development Environment
- **Host OS**: Windows (WSL path detected: `/mnt/c/Windows/system32/torch-project`)
- **IDE**: PlatformIO (ESP32), Arduino IDE (Proffie)
- **Debug Interface (SWD/JTAG)**: USB Serial (ESP32), SWD (Proffie STM32L433CC)
- **Debug Probe (ST-Link/J-Link/OpenOCD)**: USB-to-Serial (ESP32), ST-Link or compatible (Proffie)

## Debug Configuration
<!-- Auto-learned during debug sessions. Edit these directly to change debug settings. -->
- **Debug Probe**: USB Serial (ESP32 @ 115200 baud)
- **Runner Method**: PlatformIO monitor
- **GDB Server Command**: N/A (serial debugging)
- **GDB Executable**: N/A
- **Firmware ELF Path**: `.pio/build/esp32dev/firmware.elf`
- **GDB Server Port**: N/A
- **West Board**: N/A
- **West Build Directory**: N/A
- **West Venv Path**: N/A

## Power Management
- **Sleep Modes**: Not configured (ESP32 capable of light/deep sleep, not implemented)
- **Power Domains**: Single 3.3V domain (ESP32), 3.3V + 5V (Proffie for LEDs)
- **Battery / Power Source**: External power supply (assumed), battery-capable

## Boot Configuration
- **Bootloader**: ESP32 ROM bootloader + Arduino bootloader, STM32 system bootloader (Proffie)
- **OTA Mechanism**: Not implemented

## Interrupt Configuration
- **NVIC Priority Groups**: Managed by Arduino framework (ESP32), default (Proffie)
- **Key ISRs**: 
  - ESP32: UART RX interrupt, ESP-NOW receive callback
  - Proffie: I2C interrupt, Timer interrupts (assumed for audio/LED PWM)

## Compliance & Standards
- None specified

## Supporting Devices
- **Peer ESP32 device**: Second identical ESP32 unit for bidirectional motion pattern exchange via ESP-NOW

## Project Notes
This is a dual-device motion capture and playback system:
- **Proffie V3.9** captures motion from onboard LSM6DS3 IMU, generates real-time light (RGBW NeoPixel) and sound patterns, and communicates with ESP32 via UART
- **ESP32** acts as wireless relay, receiving motion data from Proffie via UART and transmitting to peer ESP32 via ESP-NOW, or receiving from peer and forwarding to Proffie
- **Gesture trigger**: Sharp shake detection on Proffie initiates transmission
- **Playback**: Receiving device plays back captured motion pattern with synchronized light and sound
- **Bidirectional**: Devices alternate between transmit and receive modes for pattern exchange

Current implementation status (per `plan-motion-capture-playback-system.md`):
- ESP32 firmware: UART handler, ESP-NOW handler, state machine implemented
- Proffie firmware: LSM6DS3 driver, shake detection, UART protocol in progress
- Shared protocol: UART packet definitions in `common/uart_protocol.h`
