#ifndef LSM6DS3_DRIVER_H
#define LSM6DS3_DRIVER_H
#include <Wire.h>
#include "uart_protocol.h"  // For IMUData struct

#define LSM6DS3_ADDR 0x6A          // SA0 pin = GND on Proffie V3.9
#define LSM6DS3_WHO_AM_I 0x0F      // Should return 0x69
#define LSM6DS3_CTRL1_XL 0x10      // Accel control
#define LSM6DS3_CTRL2_G 0x11       // Gyro control
#define LSM6DS3_CTRL3_C 0x12       // Common control
#define LSM6DS3_OUTX_L_G 0x22      // Gyro X low byte
#define LSM6DS3_OUTX_L_XL 0x28     // Accel X low byte

class LSM6DS3 {
public:
  bool begin(int sda = -1, int scl = -1);
  bool readData(IMUData* data);
  float getAccelMagnitude();  // For shake detection
private:
  uint8_t read8(uint8_t reg);
  void write8(uint8_t reg, uint8_t val);
  int16_t read16(uint8_t reg);
};
#endif
