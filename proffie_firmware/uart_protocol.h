#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>

// IMU Data Structure (shared between modules)
struct IMUData {
  int16_t ax, ay, az;  // Accel (raw)
  int16_t gx, gy, gz;  // Gyro (raw)
  uint32_t timestamp;  // millis()
};

#define UART_BAUD 115200
#define PACKET_START_BYTE 0xAA
#define PACKET_END_BYTE 0x55

enum MessageType {
  MSG_START_TRANSMIT = 0x01,
  MSG_MOTION_SAMPLE = 0x02,
  MSG_END_TRANSMIT = 0x03,
  MSG_ACK = 0x04,
  MSG_NACK = 0x05,
  MSG_INCOMING_DATA = 0x06,
  MSG_START_RECEIVE = 0x07,
  MSG_END_RECEIVE = 0x08
};

struct UARTPacket {
  uint8_t start;       // 0xAA
  uint8_t type;        // MessageType
  uint16_t length;     // Payload length
  uint8_t payload[64]; // Max 64 bytes
  uint8_t checksum;    // XOR of all bytes
  uint8_t end;         // 0x55
};

#endif
