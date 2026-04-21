#ifndef MOTION_BUFFER_H
#define MOTION_BUFFER_H
#include "lsm6ds3_driver.h"

#define BUFFER_DURATION_SEC 10
#define SAMPLE_RATE_HZ 50
#define MAX_SAMPLES (BUFFER_DURATION_SEC * SAMPLE_RATE_HZ)

class MotionBuffer {
public:
  void reset();
  bool addSample(const IMUData& data);  // Returns false if buffer full
  uint16_t getCount() const { return count; }
  const IMUData& getSample(uint16_t idx) const { return samples[idx]; }
  bool isFull() const { return count >= MAX_SAMPLES; }
  bool hasOverflowed() const { return overflowed; }
  void clearOverflowFlag() { overflowed = false; }
private:
  IMUData samples[MAX_SAMPLES];
  uint16_t count;
  bool overflowed;
};
#endif
