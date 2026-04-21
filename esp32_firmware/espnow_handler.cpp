#include "espnow_handler.h"

// Static members
ESPNowPacket ESPNowHandler::rxBuffer;
bool ESPNowHandler::rxDataAvailable = false;

void ESPNowHandler::begin() {
  // Set device as WiFi station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);
  
  // Set broadcast peer (FF:FF:FF:FF:FF:FF)
  // In production, you'd pair with specific MAC addresses
  memset(peerMAC, 0xFF, 6);
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    isPaired = false;
  } else {
    isPaired = true;
    Serial.println("ESP-NOW initialized");
  }
  
  txSequenceNum = 0;
}

void ESPNowHandler::startTransmission() {
  txSequenceNum = 0;
  
  ESPNowPacket pkt;
  pkt.type = ESP_PKT_START;
  pkt.sequenceNum = txSequenceNum++;
  pkt.totalSamples = 0;  // Will be updated as we receive data
  pkt.length = 0;
  
  esp_now_send(peerMAC, (uint8_t*)&pkt, sizeof(ESPNowPacket));
  Serial.println("Sent ESP-NOW START");
}

void ESPNowHandler::sendMotionPacket(const uint8_t* data, uint16_t len) {
  if (!isPaired) return;
  
  ESPNowPacket pkt;
  pkt.type = ESP_PKT_MOTION;
  pkt.sequenceNum = txSequenceNum++;
  pkt.totalSamples = 0;
  pkt.length = len;
  
  if (len > sizeof(pkt.payload)) {
    len = sizeof(pkt.payload);
  }
  
  memcpy(pkt.payload, data, len);
  
  esp_err_t result = esp_now_send(peerMAC, (uint8_t*)&pkt, sizeof(ESPNowPacket));
  if (result != ESP_OK) {
    Serial.printf("ESP-NOW send failed: %d\n", result);
  }
}

void ESPNowHandler::endTransmission() {
  ESPNowPacket pkt;
  pkt.type = ESP_PKT_END;
  pkt.sequenceNum = txSequenceNum++;
  pkt.totalSamples = 0;
  pkt.length = 0;
  
  esp_now_send(peerMAC, (uint8_t*)&pkt, sizeof(ESPNowPacket));
  Serial.println("Sent ESP-NOW END");
}

bool ESPNowHandler::hasReceivedData() {
  return rxDataAvailable;
}

bool ESPNowHandler::receivePacket(ESPNowPacket* pkt) {
  if (!rxDataAvailable) return false;
  
  memcpy(pkt, &rxBuffer, sizeof(ESPNowPacket));
  rxDataAvailable = false;
  
  return true;
}

void ESPNowHandler::onDataRecv(const esp_now_recv_info* recv_info, const uint8_t* data, int len) {
  if (len == sizeof(ESPNowPacket)) {
    memcpy(&rxBuffer, data, sizeof(ESPNowPacket));
    rxDataAvailable = true;
    
    Serial.printf("Received ESP-NOW packet type 0x%02X, seq %d from %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  rxBuffer.type, rxBuffer.sequenceNum,
                  recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                  recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
  }
}

void ESPNowHandler::onDataSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    // Serial.println("ESP-NOW send success");
  } else {
    Serial.println("ESP-NOW send failed");
  }
}
