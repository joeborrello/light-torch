#include "uart_handler.h"

void UARTHandler::begin(uint8_t rxPin, uint8_t txPin) {
  serial.begin(115200, SERIAL_8N1, rxPin, txPin);
  serial.setTimeout(500);  // 500ms — generous timeout for chunked writes from LPUART1
}

bool UARTHandler::available() {
  return serial.available() >= 1;  // trigger immediately; readBytes(500ms timeout) waits for full packet
}

bool UARTHandler::receivePacket(UARTPacket* pkt) {
  if (!available()) return false;
  
  // Look for start byte
  while (serial.available() > 0) {
    uint8_t b = serial.read();
    if (b == 0xAA) {
      pkt->start = b;
      break;
    }
  }
  
  if (pkt->start != 0xAA) return false;
  
  // Read rest of packet
  size_t n = serial.readBytes((uint8_t*)&pkt->type, sizeof(UARTPacket) - 1);
  if (n != sizeof(UARTPacket) - 1) {
    Serial.printf("readBytes short: got %d expected %d\n", (int)n, (int)(sizeof(UARTPacket) - 1));
    return false;
  }

  // Validate
  if (!validatePacket(pkt)) {
    sendNack();
    return false;
  }
  
  return true;
}

void UARTHandler::sendAck() {
  uint8_t ack = PKT_ACK;
  sendPacket(PKT_ACK, nullptr, 0);
}

void UARTHandler::sendNack() {
  sendPacket(PKT_NACK, nullptr, 0);
}

void UARTHandler::sendStartReceive(uint16_t sampleCount) {
  sendPacket(PKT_START_RECEIVE, (uint8_t*)&sampleCount, sizeof(sampleCount));
}

void UARTHandler::sendMotionData(const uint8_t* data, uint16_t len) {
  sendPacket(PKT_MOTION_DATA, data, len);
}

void UARTHandler::sendEndReceive() {
  sendPacket(PKT_END_RECEIVE, nullptr, 0);
}

bool UARTHandler::sendPacket(uint8_t type, const uint8_t* data, uint16_t len) {
  UARTPacket pkt;
  pkt.start = 0xAA;
  pkt.type = type;
  pkt.length = len;
  
  if (len > 0 && data != nullptr) {
    memcpy(pkt.payload, data, len);
  }
  
  pkt.checksum = calculateChecksum(&pkt);
  pkt.end = 0x55;
  
  size_t written = serial.write((uint8_t*)&pkt, sizeof(UARTPacket));
  serial.flush();
  
  return written == sizeof(UARTPacket);
}

uint8_t UARTHandler::calculateChecksum(const UARTPacket* pkt) {
  uint8_t checksum = pkt->type ^ (pkt->length & 0xFF) ^ ((pkt->length >> 8) & 0xFF);
  for (uint16_t i = 0; i < pkt->length; i++) {
    checksum ^= pkt->payload[i];
  }
  return checksum;
}

bool UARTHandler::validatePacket(const UARTPacket* pkt) {
  if (pkt->start != 0xAA) return false;
  if (pkt->end != 0x55) return false;
  if (pkt->length > 64) return false;
  
  uint8_t expectedChecksum = calculateChecksum(pkt);
  return pkt->checksum == expectedChecksum;
}
