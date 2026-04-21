#include "audio_tones.h"
#include <Arduino.h>
#include <math.h>

void AudioTones::begin() {
  pinMode(AUDIO_DAC_PIN, OUTPUT);
  isPlaying = false;
}

void AudioTones::updateFromMotion(const IMUData& data) {
  float intensity = calculateIntensity(data);
  uint16_t freq = mapIntensityToFreq(intensity);
  
  // Generate PWM tone on DAC pin
  // Note: Proffie uses analogWrite for DAC output
  // Frequency mapping: higher intensity = higher frequency
  tone(AUDIO_DAC_PIN, freq);
}

void AudioTones::playTone(uint16_t freq_hz, uint16_t duration_ms) {
  tone(AUDIO_DAC_PIN, freq_hz);
  toneStartTime = millis();
  toneDuration = duration_ms;
  isPlaying = true;
}

void AudioTones::stop() {
  noTone(AUDIO_DAC_PIN);
  isPlaying = false;
}

uint16_t AudioTones::mapIntensityToFreq(float intensity) {
  // Map 0.0-4.0g to 200-2000Hz
  intensity = constrain(intensity, 0.0, 4.0);
  return (uint16_t)(200 + (intensity / 4.0) * 1800);
}

float AudioTones::calculateIntensity(const IMUData& data) {
  // Calculate total acceleration magnitude
  float ax = data.ax / 8192.0;  // ±4g range
  float ay = data.ay / 8192.0;
  float az = data.az / 8192.0;
  return sqrt(ax*ax + ay*ay + az*az);
}

// ============================================================
// ERROR INDICATION TONES
// ============================================================

void AudioTones::playErrorTone_IMU() {
  // Low continuous buzz (200Hz) - critical hardware failure
  tone(AUDIO_DAC_PIN, 200);
  delay(1000);
  noTone(AUDIO_DAC_PIN);
}

void AudioTones::playErrorTone_UARTTimeout() {
  // Double beep (800Hz, 150ms each)
  for (int i = 0; i < 2; i++) {
    tone(AUDIO_DAC_PIN, 800);
    delay(150);
    noTone(AUDIO_DAC_PIN);
    delay(100);
  }
}

void AudioTones::playErrorTone_BufferOverflow() {
  // Triple beep (1000Hz, 100ms each)
  for (int i = 0; i < 3; i++) {
    tone(AUDIO_DAC_PIN, 1000);
    delay(100);
    noTone(AUDIO_DAC_PIN);
    delay(80);
  }
}

void AudioTones::playErrorTone_ESPNowFailed() {
  // Descending tone (1200Hz → 400Hz over 500ms)
  for (int freq = 1200; freq >= 400; freq -= 50) {
    tone(AUDIO_DAC_PIN, freq);
    delay(30);
  }
  noTone(AUDIO_DAC_PIN);
}

void AudioTones::playErrorTone_ChecksumFail() {
  // Short chirp (1500Hz, 80ms)
  tone(AUDIO_DAC_PIN, 1500);
  delay(80);
  noTone(AUDIO_DAC_PIN);
}
