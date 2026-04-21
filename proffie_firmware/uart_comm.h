#ifndef UART_COMM_H
#define UART_COMM_H
#include <HardwareSerial.h>
#include "uart_protocol.h"
#include "lsm6ds3_driver.h"

class UARTComm {
public:
  UARTComm() : serial(&Serial1) {}
  void begin(int txPin = -1, int rxPin = -1, uint32_t baud = UART_BAUD);
  bool sendStartTransmit(uint16_t sampleCount);
  bool sendMotionSample(const IMUData& sample, uint16_t index);
  bool sendEndTransmit();
  bool sendStartReceive();
  bool sendEndReceive();
  bool waitForAck(uint16_t timeout_ms);
  bool checkIncomingData();
  bool receiveMotionSample(IMUData* sample);
  
  // Error tracking
  enum ErrorType {
    NO_ERROR = 0,
    TIMEOUT_ERROR,
    CHECKSUM_ERROR,
    BUFFER_OVERFLOW
  };
  ErrorType getLastError() { return lastError; }
  void clearError() { lastError = NO_ERROR; }
  
private:
  HardwareSerial* serial;
  bool sendPacket(uint8_t type, const uint8_t* data, uint16_t len);
  bool receivePacket(UARTPacket* pkt, uint16_t timeout_ms);
  uint8_t calculateChecksum(const UARTPacket* pkt);
  ErrorType lastError = NO_ERROR;
};
#endif
