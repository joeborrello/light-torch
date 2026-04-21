#include <Arduino.h>
#include "uart_protocol.h"
#include "uart_comm.h"

// Define Serial1 for STM32 USART1 (PA9=TX, PA10=RX)
HardwareSerial Serial1(PA10, PA9);

void UARTComm::begin(int txPin, int rxPin, uint32_t baud) {
  serial = &Serial1;  // Use Serial1 on Proffie V3.9
  serial->begin(baud);
}

bool UARTComm::sendStartTransmit(uint16_t sampleCount) {
  uint8_t payload[2];
  payload[0] = sampleCount & 0xFF;
  payload[1] = (sampleCount >> 8) & 0xFF;
  return sendPacket(MSG_START_TRANSMIT, payload, 2);
}

bool UARTComm::sendMotionSample(const IMUData& sample, uint16_t index) {
  uint8_t payload[14];
  payload[0] = index & 0xFF;
  payload[1] = (index >> 8) & 0xFF;
  
  // Pack IMU data (6 int16_t values)
  memcpy(&payload[2], &sample.gx, 2);
  memcpy(&payload[4], &sample.gy, 2);
  memcpy(&payload[6], &sample.gz, 2);
  memcpy(&payload[8], &sample.ax, 2);
  memcpy(&payload[10], &sample.ay, 2);
  memcpy(&payload[12], &sample.az, 2);
  
  return sendPacket(MSG_MOTION_SAMPLE, payload, 14);
}

bool UARTComm::sendEndTransmit() {
  return sendPacket(MSG_END_TRANSMIT, nullptr, 0);
}

bool UARTComm::sendStartReceive() {
  return sendPacket(MSG_START_RECEIVE, nullptr, 0);
}

bool UARTComm::sendEndReceive() {
  return sendPacket(MSG_END_RECEIVE, nullptr, 0);
}

bool UARTComm::waitForAck(uint16_t timeout_ms) {
  UARTPacket pkt;
  if (receivePacket(&pkt, timeout_ms)) {
    return (pkt.type == MSG_ACK);
  }
  return false;
}

bool UARTComm::checkIncomingData() {
  if (serial->available() > 0) {
    uint8_t byte = serial->peek();
    if (byte == PACKET_START_BYTE) {
      UARTPacket pkt;
      if (receivePacket(&pkt, 100)) {
        return (pkt.type == MSG_INCOMING_DATA);
      }
    }
  }
  return false;
}

bool UARTComm::receiveMotionSample(IMUData* sample) {
  UARTPacket pkt;
  if (receivePacket(&pkt, 1000)) {
    if (pkt.type == MSG_MOTION_SAMPLE && pkt.length == 14) {
      // Unpack IMU data
      memcpy(&sample->gx, &pkt.payload[2], 2);
      memcpy(&sample->gy, &pkt.payload[4], 2);
      memcpy(&sample->gz, &pkt.payload[6], 2);
      memcpy(&sample->ax, &pkt.payload[8], 2);
      memcpy(&sample->ay, &pkt.payload[10], 2);
      memcpy(&sample->az, &pkt.payload[12], 2);
      sample->timestamp = millis();
      return true;
    }
  }
  return false;
}

bool UARTComm::sendPacket(uint8_t type, const uint8_t* data, uint16_t len) {
  UARTPacket pkt;
  pkt.start = PACKET_START_BYTE;
  pkt.type = type;
  pkt.length = len;
  
  if (data != nullptr && len > 0) {
    memcpy(pkt.payload, data, len);
  }
  
  pkt.checksum = calculateChecksum(&pkt);
  pkt.end = PACKET_END_BYTE;
  
  // Send packet
  serial->write(pkt.start);
  serial->write(pkt.type);
  serial->write((uint8_t)(pkt.length & 0xFF));
  serial->write((uint8_t)((pkt.length >> 8) & 0xFF));
  if (len > 0) {
    serial->write(pkt.payload, len);
  }
  serial->write(pkt.checksum);
  serial->write(pkt.end);
  
  return true;
}

bool UARTComm::receivePacket(UARTPacket* pkt, uint16_t timeout_ms) {
  uint32_t startTime = millis();
  
  // Wait for start byte
  while (millis() - startTime < timeout_ms) {
    if (serial->available() > 0) {
      uint8_t byte = serial->read();
      if (byte == PACKET_START_BYTE) {
        pkt->start = byte;
        break;
      }
    }
  }
  
  if (pkt->start != PACKET_START_BYTE) {
    lastError = TIMEOUT_ERROR;
    return false;
  }
  
  // Read type
  while (serial->available() < 1 && millis() - startTime < timeout_ms);
  if (serial->available() < 1) {
    lastError = TIMEOUT_ERROR;
    return false;
  }
  pkt->type = serial->read();
  
  // Read length (2 bytes)
  while (serial->available() < 2 && millis() - startTime < timeout_ms);
  if (serial->available() < 2) return false;
  pkt->length = serial->read();
  pkt->length |= (serial->read() << 8);
  
  // Read payload
  if (pkt->length > 0) {
    while (serial->available() < pkt->length && millis() - startTime < timeout_ms);
    if (serial->available() < pkt->length) return false;
    serial->readBytes(pkt->payload, pkt->length);
  }
  
  // Read checksum
  while (serial->available() < 1 && millis() - startTime < timeout_ms);
  if (serial->available() < 1) return false;
  pkt->checksum = serial->read();
  
  // Read end byte
  while (serial->available() < 1 && millis() - startTime < timeout_ms);
  if (serial->available() < 1) return false;
  pkt->end = serial->read();
  
  // Validate
  if (pkt->end != PACKET_END_BYTE) {
    lastError = CHECKSUM_ERROR;
    return false;
  }
  if (pkt->checksum != calculateChecksum(pkt)) {
    lastError = CHECKSUM_ERROR;
    return false;
  }
  
  lastError = NO_ERROR;
  return true;
}

uint8_t UARTComm::calculateChecksum(const UARTPacket* pkt) {
  uint8_t checksum = pkt->type;
  checksum ^= (pkt->length & 0xFF);
  checksum ^= ((pkt->length >> 8) & 0xFF);
  for (uint16_t i = 0; i < pkt->length; i++) {
    checksum ^= pkt->payload[i];
  }
  return checksum;
}
