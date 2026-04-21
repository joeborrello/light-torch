#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H
#include <Adafruit_NeoPixel.h>
#include "lsm6ds3_driver.h"

// ============================================================
// USER CONFIGURATION - Change these values to match your setup
// ============================================================
#define LED_PIN 0          // Data1 pad on Proffie (PA7)
#define NUM_LEDS 45        // **CHANGE THIS** to match your LED strip length
// ============================================================

class LEDPatterns {
public:
  void begin(int pin = LED_PIN, int numLeds = NUM_LEDS);
  void updateFromMotion(const IMUData& data);
  void setIdlePattern();
  void setRecordingPattern();
  void setTransmittingPattern();
  void setReceivingPattern();
  
  // Error indication patterns
  void setErrorPattern_IMU();           // Red flash - IMU init failed
  void setErrorPattern_UARTTimeout();   // Orange pulse - UART timeout
  void setErrorPattern_BufferOverflow();// Yellow blink - Buffer full
  void setErrorPattern_ESPNowFailed();  // Magenta flash - ESP-NOW peer not found
  void setErrorPattern_ChecksumFail();  // Cyan blink - Data corruption
  
  void show();
private:
  Adafruit_NeoPixel strip;
  uint32_t mapIntensityToColor(float intensity);
  float calculateIntensity(const IMUData& data);
  void flashColor(uint32_t color, int count, int delayMs);
};
#endif
