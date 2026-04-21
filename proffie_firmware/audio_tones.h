#ifndef AUDIO_TONES_H
#define AUDIO_TONES_H
#include "lsm6ds3_driver.h"

#define AUDIO_DAC_PIN 6  // PA4 on Proffie V3.9

class AudioTones {
public:
  void begin();
  void updateFromMotion(const IMUData& data);
  void playTone(uint16_t freq_hz, uint16_t duration_ms);
  void stop();
  
  // Error indication tones
  void playErrorTone_IMU();           // Low buzz - IMU init failed
  void playErrorTone_UARTTimeout();   // Double beep - UART timeout
  void playErrorTone_BufferOverflow();// Triple beep - Buffer full
  void playErrorTone_ESPNowFailed();  // Descending tone - ESP-NOW failed
  void playErrorTone_ChecksumFail();  // Short chirp - Data corruption
  
private:
  uint16_t mapIntensityToFreq(float intensity);
  float calculateIntensity(const IMUData& data);
  uint32_t toneStartTime;
  uint16_t toneDuration;
  bool isPlaying;
};
#endif
