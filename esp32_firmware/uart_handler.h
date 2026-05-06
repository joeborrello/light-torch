#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <HardwareSerial.h>
#include "uart_protocol.h"

class UARTHandler {
public:
  UARTHandler() : serial(Serial2) {}  // Use Arduino global Serial2 — avoids dual-ownership of UART2 peripheral
  void begin(uint8_t rxPin, uint8_t txPin);
  bool available();
  int  rawBytesAvailable()       { return serial.available(); }
  int  peekByte()                { return serial.peek(); }
  HardwareSerial& getRawSerial() { return serial; }
  bool receivePacket(UARTPacket* pkt);
  bool validatePublic(const UARTPacket* pkt) { return validatePacket(pkt); }
  void sendAck();
  void sendNack();
  void sendStartReceive(uint16_t sampleCount);
  void sendMotionData(const uint8_t* data, uint16_t len);
  void sendEndReceive();

private:
  HardwareSerial& serial;
  bool sendPacket(uint8_t type, const uint8_t* data, uint16_t len);
  uint8_t calculateChecksum(const UARTPacket* pkt);
  bool validatePacket(const UARTPacket* pkt);
};

#endif
