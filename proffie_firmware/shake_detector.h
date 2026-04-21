#ifndef SHAKE_DETECTOR_H
#define SHAKE_DETECTOR_H
#include "lsm6ds3_driver.h"

#define SHAKE_THRESHOLD_G 2.5      // 2.5g spike
#define SHAKE_COOLDOWN_MS 1000     // 1 second between shakes

class ShakeDetector {
public:
  ShakeDetector() : prevMagnitude(0), lastShakeTime(0), shakeFlag(false) {}
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
