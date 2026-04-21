# Motion Capture and Playback System for Proffie V3.9 + ESP32

## Summary
Build a dual-device system where each unit (Proffie V3.9 + ESP32) captures motion from the LSM6DS3 IMU, generates real-time light (RGBW NeoPixel) and sound patterns, records the sequence, and transmits it wirelessly via ESP-NOW when a sharp shake gesture is detected. The receiving device plays back the pattern, then switches to transmit mode for back-and-forth pattern exchange.

## Current State
- Fresh project on Windows
- No existing code
- Hardware: Proffie V3.9 boards (STM32L433CC, 80MHz, LSM6DS3 IMU @ 0x6A, 3W amp, 6 FETs), ESP32 boards
- Communication: Proffie ↔ ESP32 via UART (TX/RX pads), ESP32 ↔ ESP32 via ESP-NOW
- Target IDE: Arduino IDE with ProffieOS for Proffie, PlatformIO for ESP32
- LED strip: RGBW NeoPixels (4-wire: +5V, Data, GND)

## Steps

### Phase 1: Foundation - LSM6DS3 Driver and Shake Detection

- [x] **Step 1.1: Create project directory structure**
  - **Files**: Create folders
  - **What**: 
    - `proffie_firmware/` — Arduino sketch for Proffie V3.9
    - `esp32_firmware/` — PlatformIO project for ESP32
    - `docs/` — Wiring diagrams
    - `common/` — Shared protocol definitions (`.h` files)
  - **Why**: Organize code for two platforms with shared protocol headers

- [x] **Step 1.2: Implement LSM6DS3 driver for Proffie**
  - **File**: `proffie_firmware/lsm6ds3_driver.h` (create)
  - **What**: I2C driver for LSM6DS3 IMU on Proffie V3.9
  - **Why**: Read accelerometer and gyroscope data for motion capture and shake detection
  - **Details**:
    ```cpp
    #ifndef LSM6DS3_DRIVER_H
    #define LSM6DS3_DRIVER_H
    #include <Wire.h>
    
    #define LSM6DS3_ADDR 0x6A          // SA0 pin = GND on Proffie V3.9
    #define LSM6DS3_WHO_AM_I 0x0F      // Should return 0x69
    #define LSM6DS3_CTRL1_XL 0x10      // Accel control
    #define LSM6DS3_CTRL2_G 0x11       // Gyro control
    #define LSM6DS3_CTRL3_C 0x12       // Common control
    #define LSM6DS3_OUTX_L_G 0x22      // Gyro X low byte
    #define LSM6DS3_OUTX_L_XL 0x28     // Accel X low byte
    
    struct IMUData {
      int16_t ax, ay, az;  // Accel (raw)
      int16_t gx, gy, gz;  // Gyro (raw)
      uint32_t timestamp;  // millis()
    };
    
    class LSM6DS3 {
    public:
      bool begin();
      bool readData(IMUData* data);
      float getAccelMagnitude();  // For shake detection
    private:
      uint8_t read8(uint8_t reg);
      void write8(uint8_t reg, uint8_t val);
      int16_t read16(uint8_t reg);
    };
    #endif
    ```
    - Configure: Accel ±4g @ 104Hz, Gyro ±500dps @ 104Hz
    - Use I2C1 (Wire) on Proffie's onboard I2C bus
    - Verify WHO_AM_I = 0x69 on init

- [x] **Step 1.3: Implement LSM6DS3 driver implementation**
  - **File**: `proffie_firmware/lsm6ds3_driver.cpp` (create)
  - **What**: Implementation of I2C read/write and sensor configuration
  - **Details**:
    ```cpp
    #include "lsm6ds3_driver.h"
    
    bool LSM6DS3::begin() {
      Wire.begin();
      Wire.setClock(400000);  // 400kHz I2C
      
      if (read8(LSM6DS3_WHO_AM_I) != 0x69) return false;
      
      // Accel: 104Hz, ±4g, high-performance mode
      write8(LSM6DS3_CTRL1_XL, 0x40);  // 0100 0000
      // Gyro: 104Hz, ±500dps
      write8(LSM6DS3_CTRL2_G, 0x44);   // 0100 0100
      // Enable auto-increment for multi-byte reads
      write8(LSM6DS3_CTRL3_C, 0x04);
      
      delay(100);  // Sensor startup time
      return true;
    }
    
    bool LSM6DS3::readData(IMUData* data) {
      Wire.beginTransmission(LSM6DS3_ADDR);
      Wire.write(LSM6DS3_OUTX_L_G);
      if (Wire.endTransmission(false) != 0) return false;
      
      Wire.requestFrom(LSM6DS3_ADDR, (uint8_t)12);
      if (Wire.available() < 12) return false;
      
      data->gx = Wire.read() | (Wire.read() << 8);
      data->gy = Wire.read() | (Wire.read() << 8);
      data->gz = Wire.read() | (Wire.read() << 8);
      
      Wire.beginTransmission(LSM6DS3_ADDR);
      Wire.write(LSM6DS3_OUTX_L_XL);
      Wire.endTransmission(false);
      Wire.requestFrom(LSM6DS3_ADDR, (uint8_t)6);
      
      data->ax = Wire.read() | (Wire.read() << 8);
      data->ay = Wire.read() | (Wire.read() << 8);
      data->az = Wire.read() | (Wire.read() << 8);
      data->timestamp = millis();
      return true;
    }
    
    float LSM6DS3::getAccelMagnitude() {
      IMUData data;
      if (!readData(&data)) return 0.0;
      // Convert to g (±4g range, 16-bit)
      float ax_g = data.ax / 8192.0;
      float ay_g = data.ay / 8192.0;
      float az_g = data.az / 8192.0;
      return sqrt(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
    }
    ```

- [x] **Step 1.4: Implement shake detection algorithm**
  - **File**: `proffie_firmware/shake_detector.h` (create)
  - **What**: Detect sharp shake gestures using accelerometer magnitude
  - **Why**: Trigger transmission when user shakes the device
  - **Details**:
    ```cpp
    #ifndef SHAKE_DETECTOR_H
    #define SHAKE_DETECTOR_H
    #include "lsm6ds3_driver.h"
    
    #define SHAKE_THRESHOLD_G 2.5      // 2.5g spike
    #define SHAKE_COOLDOWN_MS 1000     // 1 second between shakes
    
    class ShakeDetector {
    public:
      void update(const IMUData& data);
      bool isShakeDetected();
      void reset();
    private:
      float prevMagnitude;
      uint32_t lastShakeTime;
      bool shakeFlag;
      float calculateMagnitude(const IMUData& data);
    };
    #endif
    ```
    - Detect sudden acceleration spike > 2.5g
    - Use high-pass filter to remove gravity
    - Cooldown period to prevent double-triggers

- [x] **Step 1.5: Implement shake detector implementation**
  - **File**: `proffie_firmware/shake_detector.cpp` (create)
  - **What**: Shake detection logic with magnitude calculation
  - **Details**:
    ```cpp
    #include "shake_detector.h"
    
    void ShakeDetector::update(const IMUData& data) {
      float mag = calculateMagnitude(data);
      
      // High-pass filter: detect change from baseline (1g gravity)
      float delta = abs(mag - 1.0);
      
      if (delta > SHAKE_THRESHOLD_G) {
        uint32_t now = millis();
        if (now - lastShakeTime > SHAKE_COOLDOWN_MS) {
          shakeFlag = true;
          lastShakeTime = now;
        }
      }
      prevMagnitude = mag;
    }
    
    bool ShakeDetector::isShakeDetected() {
      bool result = shakeFlag;
      shakeFlag = false;  // Clear flag after read
      return result;
    }
    
    float ShakeDetector::calculateMagnitude(const IMUData& data) {
      float ax = data.ax / 8192.0;  // ±4g range
      float ay = data.ay / 8192.0;
      float az = data.az / 8192.0;
      return sqrt(ax*ax + ay*ay + az*az);
    }
    ```

### Phase 2: Motion Capture and Pattern Generation

- [x] **Step 2.1: Implement motion data buffer**
  - **File**: `proffie_firmware/motion_buffer.h` (create)
  - **What**: Circular buffer to store motion samples
  - **Why**: Record motion data before shake-triggered transmission
  - **Details**:
    ```cpp
    #ifndef MOTION_BUFFER_H
    #define MOTION_BUFFER_H
    #include "lsm6ds3_driver.h"
    
    #define BUFFER_DURATION_SEC 10
    #define SAMPLE_RATE_HZ 50
    #define MAX_SAMPLES (BUFFER_DURATION_SEC * SAMPLE_RATE_HZ)
    
    class MotionBuffer {
    public:
      void reset();
      void addSample(const IMUData& data);
      uint16_t getCount() const { return count; }
      const IMUData& getSample(uint16_t idx) const { return samples[idx]; }
      bool isFull() const { return count >= MAX_SAMPLES; }
    private:
      IMUData samples[MAX_SAMPLES];
      uint16_t count;
    };
    #endif
    ```
    - Store 10 seconds @ 50Hz = 500 samples
    - Each sample: 12 bytes (6 int16_t) + 4 bytes timestamp = 16 bytes
    - Total: 8KB RAM

- [x] **Step 2.2: Implement RGBW NeoPixel pattern generator**
  - **File**: `proffie_firmware/led_patterns.h` (create)
  - **What**: Generate RGBW LED patterns based on motion intensity
  - **Why**: Visual feedback during recording and playback
  - **Details**:
    ```cpp
    #ifndef LED_PATTERNS_H
    #define LED_PATTERNS_H
    #include <Adafruit_NeoPixel.h>
    #include "lsm6ds3_driver.h"
    
    #define LED_PIN 0          // Data1 pad (PA7)
    #define NUM_LEDS 60        // Adjust to strip length
    
    class LEDPatterns {
    public:
      void begin();
      void updateFromMotion(const IMUData& data);
      void setIdlePattern();
      void setRecordingPattern();
      void setTransmittingPattern();
      void show();
    private:
      Adafruit_NeoPixel strip;
      uint32_t mapIntensityToColor(float intensity);
    };
    #endif
    ```
    - Use SK6812 RGBW mode (NEO_GRBW + NEO_KHZ800)
    - Map gyro magnitude to color: low=blue, med=green, high=red+white
    - Smooth with exponential moving average (alpha=0.3)

- [x] **Step 2.3: Implement audio tone generator**
  - **File**: `proffie_firmware/audio_tones.h` (create)
  - **What**: Generate audio tones using Proffie's DAC and amplifier
  - **Why**: Audio feedback during motion capture and playback
  - **Details**:
    ```cpp
    #ifndef AUDIO_TONES_H
    #define AUDIO_TONES_H
    #include "lsm6ds3_driver.h"
    
    class AudioTones {
    public:
      void begin();
      void updateFromMotion(const IMUData& data);
      void playTone(uint16_t freq_hz, uint16_t duration_ms);
      void stop();
    private:
      uint16_t mapIntensityToFreq(float intensity);
    };
    #endif
    ```
    - Use analogWrite() on DAC pin (PA4, pin 6)
    - Map motion intensity to frequency: 200Hz - 2000Hz
    - Use Proffie's built-in 3W amplifier

### Phase 3: UART Communication Protocol

- [x] **Step 3.1: Define UART protocol**
  - **File**: `common/uart_protocol.h` (create)
  - **What**: Shared protocol definition for Proffie ↔ ESP32 UART
  - **Why**: Both devices need identical message format
  - **Details**:
    ```cpp
    #ifndef UART_PROTOCOL_H
    #define UART_PROTOCOL_H
    
    #define UART_BAUD 115200
    #define PACKET_START_BYTE 0xAA
    #define PACKET_END_BYTE 0x55
    
    enum MessageType {
      MSG_START_TRANSMIT = 0x01,
      MSG_MOTION_SAMPLE = 0x02,
      MSG_END_TRANSMIT = 0x03,
      MSG_ACK = 0x04,
      MSG_NACK = 0x05,
      MSG_INCOMING_DATA = 0x06
    };
    
    struct UARTPacket {
      uint8_t start;       // 0xAA
      uint8_t type;        // MessageType
      uint16_t length;     // Payload length
      uint8_t payload[64]; // Max 64 bytes
      uint8_t checksum;    // XOR of all bytes
      uint8_t end;         // 0x55
    };
    
    #endif
    ```
    - Frame format: START | TYPE | LEN | PAYLOAD | CHECKSUM | END
    - Checksum: XOR of type, length, and payload bytes
    - Max payload: 64 bytes (fits 4 IMU samples per packet)

- [x] **Step 3.2: Implement UART driver for Proffie**
  - **File**: `proffie_firmware/uart_comm.h` (create)
  - **What**: UART communication layer for Proffie (master)
  - **Why**: Send motion data to ESP32 for ESP-NOW transmission
  - **Details**:
    ```cpp
    #ifndef UART_COMM_H
    #define UART_COMM_H
    #include <HardwareSerial.h>
    #include "../common/uart_protocol.h"
    #include "motion_buffer.h"
    
    class UARTComm {
    public:
      void begin();
      bool sendStartTransmit(uint16_t sampleCount);
      bool sendMotionSample(const IMUData& sample, uint16_t index);
      bool sendEndTransmit();
      bool waitForAck(uint16_t timeout_ms);
      bool checkIncomingData();
      bool receiveMotionSample(IMUData* sample);
    private:
      HardwareSerial* serial;
      bool sendPacket(uint8_t type, const uint8_t* data, uint16_t len);
      bool receivePacket(UARTPacket* pkt, uint16_t timeout_ms);
      uint8_t calculateChecksum(const UARTPacket* pkt);
    };
    #endif
    ```
    - Use Serial1 (TX/RX pads on Proffie V3.9)
    - TX pad → ESP32 RX, RX pad → ESP32 TX
    - 115200 baud, 8N1

- [x] **Step 3.3: Update wiring documentation**
  - **File**: `docs/wiring.md` (create)
  - **What**: Complete wiring diagram for Proffie + ESP32
  - **Details**:
    ```
    PROFFIE V3.9 → ESP32 CONNECTIONS:
    
    UART (Serial1):
    - TX pad (PA9) → ESP32 GPIO16 (RX2)
    - RX pad (PA10) → ESP32 GPIO17 (TX2)
    - GND → ESP32 GND
    
    Power:
    - BATT+ ← 3.7V Li-ion battery
    - GND ← Battery negative
    - 3.3V → ESP32 VIN (if powering ESP32 from Proffie)
    
    RGBW NeoPixel Strip:
    - Data1 (PA7, pin 0) → LED strip DIN
    - 5V (from boost converter) → LED strip VCC
    - BATT- → LED strip GND
    
    Audio:
    - Speaker+ / Speaker- → 4-8Ω speaker
    
    LSM6DS3 IMU:
    - Onboard, I2C address 0x6A (SA0=GND)
    ```

### Phase 4: ESP32 Firmware - UART Bridge and ESP-NOW

- [x] **Step 4.1: Create ESP32 PlatformIO project**
  - **File**: `esp32_firmware/platformio.ini` (create)
  - **What**: PlatformIO configuration
  - **Details**:
    ```ini
    [env:esp32dev]
    platform = espressif32
    board = esp32dev
    framework = arduino
    monitor_speed = 115200
    lib_deps = 
        Wire
    ```

- [x] **Step 4.2: Implement UART receiver for ESP32**
  - **File**: `esp32_firmware/src/uart_handler.h` (create)
  - **What**: UART communication handler (slave side)
  - **Why**: Receive motion data from Proffie and forward via ESP-NOW
  - **Details**:
    ```cpp
    #ifndef UART_HANDLER_H
    #define UART_HANDLER_H
    #include <HardwareSerial.h>
    #include "../../common/uart_protocol.h"
    
    class UARTHandler {
    public:
      void begin();
      void update();  // Call in loop()
      bool hasIncomingData();
      bool readMotionSample(IMUData* sample);
    private:
      HardwareSerial serial;
      void handlePacket(const UARTPacket* pkt);
      void sendAck();
      void sendNack();
    };
    #endif
    ```
    - Use Serial2 (GPIO16=RX, GPIO17=TX)
    - Parse incoming packets, validate checksum
    - Send ACK/NACK responses

- [x] **Step 4.3: Implement ESP-NOW transmitter**
  - **File**: `esp32_firmware/src/espnow_tx.h` (create)
  - **What**: ESP-NOW transmission to peer device
  - **Why**: Wireless transmission of motion data
  - **Details**:
    ```cpp
    #ifndef ESPNOW_TX_H
    #define ESPNOW_TX_H
    #include <esp_now.h>
    #include <WiFi.h>
    #include "../../common/uart_protocol.h"
    
    class ESPNowTX {
    public:
      void begin(const uint8_t* peerMAC);
      bool sendMotionData(const uint8_t* data, uint16_t len);
      bool isTransmitComplete();
    private:
      static void onSent(const uint8_t* mac, esp_now_send_status_t status);
      uint8_t peerAddress[6];
      bool txComplete;
    };
    #endif
    ```
    - Set WiFi to STA mode, channel 1
    - Add peer MAC address (hardcoded or from EEPROM)
    - Max payload: 250 bytes per ESP-NOW packet

- [x] **Step 4.4: Implement ESP-NOW receiver**
  - **File**: `esp32_firmware/src/espnow_rx.h` (create)
  - **What**: ESP-NOW reception from peer device
  - **Why**: Receive motion data for playback
  - **Details**:
    ```cpp
    #ifndef ESPNOW_RX_H
    #define ESPNOW_RX_H
    #include <esp_now.h>
    #include "../../common/uart_protocol.h"
    
    class ESPNowRX {
    public:
      void begin();
      bool hasData();
      bool readMotionData(uint8_t* buffer, uint16_t* len);
    private:
      static void onReceive(const uint8_t* mac, const uint8_t* data, int len);
      static uint8_t rxBuffer[250];
      static uint16_t rxLength;
      static bool dataReady;
    };
    #endif
    ```
    - Register receive callback
    - Store incoming data in buffer
    - Forward to Proffie via UART

- [x] **Step 4.5: Implement ESP32 main loop**
  - **File**: `esp32_firmware/src/main.cpp` (create)
  - **What**: Main state machine for ESP32
  - **Why**: Coordinate UART ↔ ESP-NOW bridging
  - **Details**:
    ```cpp
    #include "uart_handler.h"
    #include "espnow_tx.h"
    #include "espnow_rx.h"
    
    UARTHandler uart;
    ESPNowTX espTx;
    ESPNowRX espRx;
    
    void setup() {
      uart.begin();
      espTx.begin(PEER_MAC_ADDRESS);  // Define in config
      espRx.begin();
    }
    
    void loop() {
      uart.update();
      
      // Forward UART → ESP-NOW
      if (uart.hasIncomingData()) {
        IMUData sample;
        if (uart.readMotionSample(&sample)) {
          espTx.sendMotionData((uint8_t*)&sample, sizeof(sample));
        }
      }
      
      // Forward ESP-NOW → UART
      if (espRx.hasData()) {
        uint8_t buffer[250];
        uint16_t len;
        if (espRx.readMotionData(buffer, &len)) {
          // Send to Proffie via UART
        }
      }
    }
    ```

### Phase 5: Integration and State Machine

- [x] **Step 5.1: Implement Proffie main state machine**
  - **File**: `proffie_firmware/proffie_firmware.ino` (create)
  - **What**: Main Arduino sketch with state machine
  - **Why**: Coordinate IMU, LEDs, audio, UART, and shake detection
  - **Details**:
    ```cpp
    #include "lsm6ds3_driver.h"
    #include "shake_detector.h"
    #include "motion_buffer.h"
    #include "led_patterns.h"
    #include "audio_tones.h"
    #include "uart_comm.h"
    
    LSM6DS3 imu;
    ShakeDetector shakeDetector;
    MotionBuffer motionBuffer;
    LEDPatterns leds;
    AudioTones audio;
    UARTComm uart;
    
    enum State { IDLE, RECORDING, TRANSMITTING, RECEIVING, PLAYBACK };
    State state = IDLE;
    
    void setup() {
      Serial.begin(115200);
      if (!imu.begin()) {
        Serial.println("LSM6DS3 init failed!");
        while(1);
      }
      leds.begin();
      audio.begin();
      uart.begin();
      Serial.println("Proffie ready");
    }
    
    void loop() {
      static uint32_t lastSample = 0;
      uint32_t now = millis();
      
      // Sample IMU at 50Hz
      if (now - lastSample >= 20) {
        lastSample = now;
        IMUData data;
        if (imu.readData(&data)) {
          shakeDetector.update(data);
          
          if (state == RECORDING) {
            motionBuffer.addSample(data);
            leds.updateFromMotion(data);
            audio.updateFromMotion(data);
          }
        }
      }
      
      // State machine
      switch (state) {
        case IDLE:
          leds.setIdlePattern();
          state = RECORDING;  // Auto-start recording
          motionBuffer.reset();
          break;
          
        case RECORDING:
          leds.setRecordingPattern();
          if (shakeDetector.isShakeDetected()) {
            state = TRANSMITTING;
            Serial.println("Shake detected! Transmitting...");
          }
          break;
          
        case TRANSMITTING:
          leds.setTransmittingPattern();
          transmitMotionData();
          state = RECEIVING;
          break;
          
        case RECEIVING:
          if (uart.checkIncomingData()) {
            state = PLAYBACK;
          }
          break;
          
        case PLAYBACK:
          playbackMotionData();
          state = IDLE;
          break;
      }
      
      leds.show();
    }
    
    void transmitMotionData() {
      uint16_t count = motionBuffer.getCount();
      uart.sendStartTransmit(count);
      
      for (uint16_t i = 0; i < count; i++) {
        const IMUData& sample = motionBuffer.getSample(i);
        uart.sendMotionSample(sample, i);
        uart.waitForAck(100);
      }
      
      uart.sendEndTransmit();
    }
    
    void playbackMotionData() {
      // Receive and play back motion samples
      IMUData sample;
      while (uart.receiveMotionSample(&sample)) {
        leds.updateFromMotion(sample);
        audio.updateFromMotion(sample);
        leds.show();
        delay(20);  // 50Hz playback
      }
    }
    ```

- [x] **Step 5.2: Add configuration header**
  - **File**: `proffie_firmware/config.h` (create)
  - **What**: Centralized configuration constants
  - **Details**:
    ```cpp
    #ifndef CONFIG_H
    #define CONFIG_H
    
    // IMU settings
    #define IMU_SAMPLE_RATE_HZ 50
    #define SHAKE_THRESHOLD_G 2.5
    #define SHAKE_COOLDOWN_MS 1000
    
    // Buffer settings
    #define RECORDING_DURATION_SEC 10
    #define MAX_SAMPLES (IMU_SAMPLE_RATE_HZ * RECORDING_DURATION_SEC)
    
    // LED settings
    #define NUM_LEDS 60
    #define LED_BRIGHTNESS 128  // 0-255
    
    // UART settings
    #define UART_BAUD 115200
    #define UART_TIMEOUT_MS 1000
    
    // ESP32 peer MAC address (update after pairing)
    #define PEER_MAC {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF}
    
    #endif
    ```

## Files Involved

| File | Action | Purpose |
|------|--------|---------|
| `proffie_firmware/lsm6ds3_driver.h` | Create | LSM6DS3 I2C driver header |
| `proffie_firmware/lsm6ds3_driver.cpp` | Create | LSM6DS3 implementation |
| `proffie_firmware/shake_detector.h` | Create | Shake detection algorithm |
| `proffie_firmware/shake_detector.cpp` | Create | Shake detector implementation |
| `proffie_firmware/motion_buffer.h` | Create | Motion data circular buffer |
| `proffie_firmware/led_patterns.h` | Create | RGBW NeoPixel pattern generator |
| `proffie_firmware/audio_tones.h` | Create | Audio tone generator |
| `proffie_firmware/uart_comm.h` | Create | UART communication (Proffie side) |
| `proffie_firmware/config.h` | Create | Configuration constants |
| `proffie_firmware/proffie_firmware.ino` | Create | Main Arduino sketch |
| `common/uart_protocol.h` | Create | Shared UART protocol definition |
| `esp32_firmware/platformio.ini` | Create | PlatformIO config |
| `esp32_firmware/src/uart_handler.h` | Create | UART handler (ESP32 side) |
| `esp32_firmware/src/espnow_tx.h` | Create | ESP-NOW transmitter |
| `esp32_firmware/src/espnow_rx.h` | Create | ESP-NOW receiver |
| `esp32_firmware/src/main.cpp` | Create | ESP32 main loop |
| `docs/wiring.md` | Create | Wiring diagram |
| `SEB.MD` | Modify | Update with hardware details |

## Open Questions

1. **ESP32 peer MAC address pairing**: How should devices discover each other? Options:
   - Hardcode MAC addresses in firmware
   - Implement pairing mode with button press
   - Use broadcast mode (less secure)

2. **Power supply for ESP32**: Should ESP32 be powered from:
   - Proffie's 3.3V regulator (check current capacity)
   - Separate battery/regulator
   - USB during development

3. **LED strip length**: How many RGBW LEDs in the strip? Affects:
   - Power consumption calculation
   - Pattern design
   - `NUM_LEDS` constant

4. **Playback synchronization**: Should playback timing be:
   - Based on recorded timestamps (exact replay)
   - Fixed 50Hz rate (simpler, may drift)

5. **Error handling**: What should happen if:
   - UART transmission fails (timeout, checksum error)
   - ESP-NOW peer not found
   - Motion buffer overflows during long recording
