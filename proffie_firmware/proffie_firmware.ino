// ============================================================
// Motion Capture & Playback System - Proffie Firmware
// ============================================================
// To change LED strip length: edit NUM_LEDS in led_patterns.h
// To adjust shake sensitivity: edit SHAKE_THRESHOLD in shake_detector.h
// To change buffer size: edit BUFFER_SIZE in motion_buffer.h
// ============================================================

#include "config.h"
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

enum State {
  IDLE,
  RECORDING,
  TRANSMITTING,
  RECEIVING,
  PLAYBACK
};

State state = IDLE;
uint32_t recordingStartTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Proffie Motion Capture System ===");
  
  // Initialize LSM6DS3 IMU
  if (!imu.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.println("ERROR: LSM6DS3 init failed!");
    // Critical error - show solid red LEDs and low buzz
    leds.begin(LED_DATA_PIN, NUM_LEDS);
    audio.begin();
    while(1) {
      leds.setErrorPattern_IMU();
      leds.show();
      audio.playErrorTone_IMU();
      delay(2000);
    }
  }
  Serial.println("LSM6DS3 initialized");
  
  // Initialize LED patterns
  leds.begin(LED_DATA_PIN, NUM_LEDS);
  Serial.println("LED patterns initialized");
  
  // Initialize audio
  audio.begin();
  Serial.println("Audio initialized");
  
  // Initialize UART communication
  uart.begin(UART_TX_PIN, UART_RX_PIN, UART_BAUD);
  Serial.println("UART initialized");
  
  Serial.println("System ready - starting in RECORDING mode");
  state = RECORDING;
  recordingStartTime = millis();
  motionBuffer.reset();
}

void loop() {
  static uint32_t lastSample = 0;
  uint32_t now = millis();
  
  // Sample IMU at 50Hz
  if (now - lastSample >= (1000 / IMU_SAMPLE_RATE_HZ)) {
    lastSample = now;
    IMUData data;
    
    if (imu.readData(&data)) {
      shakeDetector.update(data);
      
      if (state == RECORDING) {
        // Add sample to buffer
        if (!motionBuffer.addSample(data)) {
          // Buffer overflow - show error once
          if (motionBuffer.hasOverflowed()) {
            Serial.println("WARNING: Motion buffer overflow!");
            leds.setErrorPattern_BufferOverflow();
            leds.show();
            audio.playErrorTone_BufferOverflow();
            delay(500);  // Brief pause to show error
            motionBuffer.clearOverflowFlag();
          }
        }
        
        // Update LED and audio in real-time
        leds.updateFromMotion(data);
        audio.updateFromMotion(data);
        
        // Check if recording duration exceeded
        if ((now - recordingStartTime) >= (RECORDING_DURATION_SEC * 1000)) {
          Serial.println("Recording buffer full - waiting for shake to transmit");
        }
      }
      else if (state == PLAYBACK) {
        // During playback, use received data for LED/audio
        // (handled in playbackMotionData function)
      }
    }
  }
  
  // State machine
  switch (state) {
    case IDLE:
      leds.setIdlePattern();
      // Auto-transition to recording
      state = RECORDING;
      recordingStartTime = millis();
      motionBuffer.reset();
      Serial.println("State: RECORDING");
      break;
      
    case RECORDING:
      leds.setRecordingPattern();
      
      // Check for shake gesture to trigger transmission
      if (shakeDetector.isShakeDetected()) {
        state = TRANSMITTING;
        Serial.println("Shake detected! State: TRANSMITTING");
      }
      break;
      
    case TRANSMITTING:
      leds.setTransmittingPattern();
      transmitMotionData();
      
      // After transmission, go to receiving mode
      state = RECEIVING;
      Serial.println("Transmission complete. State: RECEIVING");
      break;
      
    case RECEIVING:
      leds.setReceivingPattern();
      
      // Check if incoming data available from ESP32
      if (uart.checkIncomingData()) {
        state = PLAYBACK;
        Serial.println("Incoming data detected. State: PLAYBACK");
      }
      
      // Timeout after 30 seconds, return to recording
      static uint32_t receivingStartTime = millis();
      if ((millis() - receivingStartTime) > 30000) {
        Serial.println("Receive timeout. State: RECORDING");
        state = RECORDING;
        recordingStartTime = millis();
        motionBuffer.reset();
      }
      break;
      
    case PLAYBACK:
      playbackMotionData();
      
      // After playback, return to recording mode
      state = RECORDING;
      recordingStartTime = millis();
      motionBuffer.reset();
      Serial.println("Playback complete. State: RECORDING");
      break;
  }
  
  // Update LED display
  leds.show();
}

void transmitMotionData() {
  uint16_t count = motionBuffer.getCount();
  Serial.print("Transmitting ");
  Serial.print(count);
  Serial.println(" samples...");
  
  // Send start transmission command
  uart.sendStartTransmit(count);
  delay(10);
  
  // Send all motion samples
  for (uint16_t i = 0; i < count; i++) {
    const IMUData& sample = motionBuffer.getSample(i);
    uart.sendMotionSample(sample, i);
    
    // Wait for ACK with timeout
    if (!uart.waitForAck(UART_TIMEOUT_MS)) {
      Serial.print("ACK timeout at sample ");
      Serial.println(i);
      
      // Show UART timeout error
      leds.setErrorPattern_UARTTimeout();
      leds.show();
      audio.playErrorTone_UARTTimeout();
      
      // Abort transmission after 3 consecutive timeouts
      static uint8_t timeoutCount = 0;
      timeoutCount++;
      if (timeoutCount >= 3) {
        Serial.println("Too many timeouts - aborting transmission");
        timeoutCount = 0;
        return;
      }
    } else {
      // Reset timeout counter on successful ACK
      static uint8_t timeoutCount = 0;
      timeoutCount = 0;
    }
    
    // Visual feedback during transmission
    if (i % 50 == 0) {
      Serial.print(".");
    }
  }
  Serial.println();
  
  // Send end transmission command
  uart.sendEndTransmit();
  Serial.println("Transmission complete");
}

void playbackMotionData() {
  Serial.println("Playing back received motion data...");
  
  IMUData sample;
  uint16_t sampleCount = 0;
  
  // Receive and play back motion samples at 50Hz
  while (uart.receiveMotionSample(&sample)) {
    // Check for UART errors
    if (uart.getLastError() == UARTComm::CHECKSUM_ERROR) {
      Serial.println("Checksum error detected!");
      leds.setErrorPattern_ChecksumFail();
      leds.show();
      audio.playErrorTone_ChecksumFail();
      uart.clearError();
      continue;  // Skip corrupted sample
    }
    
    // Update LED and audio based on received motion
    leds.updateFromMotion(sample);
    audio.updateFromMotion(sample);
    leds.show();
    
    sampleCount++;
    
    // Maintain 50Hz playback rate
    delay(1000 / IMU_SAMPLE_RATE_HZ);
  }
  
  Serial.print("Playback complete: ");
  Serial.print(sampleCount);
  Serial.println(" samples");
}
