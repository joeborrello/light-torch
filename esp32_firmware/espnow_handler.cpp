#include "espnow_handler.h"
#include <esp_wifi.h>

// Static members
ESPNowPacket ESPNowHandler::rxBuffer;
bool ESPNowHandler::rxDataAvailable = false;

void ESPNowHandler::begin() {
  WiFi.mode(WIFI_STA);

  // Wait for the WiFi stack to assign a valid MAC (avoids all-zeros MAC on first boot)
  uint32_t t0 = millis();
  while (WiFi.macAddress() == "00:00:00:00:00:00" && millis() - t0 < 3000) {
    delay(10);
  }
  Serial.printf("ESP-NOW MAC: %s\n", WiFi.macAddress().c_str());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  // Set channel AFTER esp_now_init so it is not overridden during init
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  esp_now_register_recv_cb(onDataRecv);

  memset(peerMAC, 0xFF, 6);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMAC, 6);
  peerInfo.channel = 1;
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
  pkt.totalSamples = 0;
  pkt.length = 0;

  esp_now_send(peerMAC, (uint8_t*)&pkt, sizeof(ESPNowPacket));
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
  }
}
