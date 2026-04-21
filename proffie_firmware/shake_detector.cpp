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

void ShakeDetector::reset() {
  shakeFlag = false;
  lastShakeTime = 0;
  prevMagnitude = 0;
}

float ShakeDetector::calculateMagnitude(const IMUData& data) {
  float ax = data.ax / 8192.0;  // ±4g range
  float ay = data.ay / 8192.0;
  float az = data.az / 8192.0;
  return sqrt(ax*ax + ay*ay + az*az);
}
