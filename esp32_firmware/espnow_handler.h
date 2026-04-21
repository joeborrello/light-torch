#ifndef ESPNOW_HANDLER_H
#define ESPNOW_HANDLER_H

#include <esp_now.h>
#include <WiFi.h>

// ESP-NOW packet types
#define ESP_PKT_START  0x10
#define ESP_PKT_MOTION 0x11
#define ESP_PKT_END    0x12

// ESP-NOW packet structure
struct ESPNowPacket {
  uint8_t type;
  uint16_t sequenceNum;
  uint16_t totalSamples;
  uint16_t length;
  uint8_t payload[200];  // Larger payload for ESP-NOW (max 250 bytes)
};

class ESPNowHandler {
public:
  void begin();
  void startTransmission();
  void sendMotionPacket(const uint8_t* data, uint16_t len);
  void endTransmission();
  bool hasReceivedData();
  bool receivePacket(ESPNowPacket* pkt);
  
private:
  static void onDataRecv(const esp_now_recv_info* recv_info, const uint8_t* data, int len);
  static void onDataSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t status);
  
  uint8_t peerMAC[6];
  uint16_t txSequenceNum;
  bool isPaired;
  
  static ESPNowPacket rxBuffer;
  static bool rxDataAvailable;
};

#endif
