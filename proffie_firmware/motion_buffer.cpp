#include "motion_buffer.h"

void MotionBuffer::reset() {
  count = 0;
  overflowed = false;
}

bool MotionBuffer::addSample(const IMUData& data) {
  if (count < MAX_SAMPLES) {
    samples[count++] = data;
    return true;
  } else {
    // Buffer full - set overflow flag
    overflowed = true;
    
    // Circular buffer: shift left and add new sample at end
    for (uint16_t i = 0; i < MAX_SAMPLES - 1; i++) {
      samples[i] = samples[i + 1];
    }
    samples[MAX_SAMPLES - 1] = data;
    return false;
  }
}
