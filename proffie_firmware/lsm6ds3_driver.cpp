#include "lsm6ds3_driver.h"

bool LSM6DS3::begin(int sda, int scl) {
  if (sda >= 0 && scl >= 0) {
    Wire.begin(sda, scl);
  } else {
    Wire.begin();
  }
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

uint8_t LSM6DS3::read8(uint8_t reg) {
  Wire.beginTransmission(LSM6DS3_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(LSM6DS3_ADDR, (uint8_t)1);
  return Wire.read();
}

void LSM6DS3::write8(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(LSM6DS3_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

int16_t LSM6DS3::read16(uint8_t reg) {
  Wire.beginTransmission(LSM6DS3_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(LSM6DS3_ADDR, (uint8_t)2);
  uint8_t low = Wire.read();
  uint8_t high = Wire.read();
  return (int16_t)((high << 8) | low);
}
