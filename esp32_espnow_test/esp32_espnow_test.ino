/*
 * ESP-NOW Ping/Pong Test
 * Flash identical firmware to BOTH Arduino Nano ESP32 boards.
 * Each board broadcasts a PING every 3 seconds and replies PONG to any PING it receives.
 * Use this to confirm ESP-NOW works before integrating with the Proffie UART chain.
 *
 * Expected Serial Monitor output (115200 baud):
 *   ESP-NOW Test Ready. My MAC: XX:XX:XX:XX:XX:XX
 *   Sent PING #1
 *   Received PING #1 from AA:BB:CC:DD:EE:FF  → Sent PONG #1
 *   Received PONG #1 from AA:BB:CC:DD:EE:FF
 *   Sent PING #2
 *   ...
 *
 * If you see "Sent PING" but no "Received" on either board → ESP-NOW link not working.
 * If you see "ESP-NOW init failed" → WiFi or ESP-NOW hardware issue.
 */

#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 13

// Broadcast address — reaches all ESP-NOW peers without pairing
static uint8_t broadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct TestPacket {
  char     type[5];   // "PING\0" or "PONG\0"
  uint32_t counter;
  uint8_t  senderMAC[6];
};

static uint32_t pingCounter  = 0;
static uint32_t lastPingSent = 0;
static bool     espnowReady  = false;

// ── Callbacks ─────────────────────────────────────────────────────────────────

void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("Send failed — check peer is in range");
  }
}

void onDataReceived(const esp_now_recv_info* info, const uint8_t* data, int len) {
  if (len != sizeof(TestPacket)) {
    Serial.printf("Unexpected packet length %d (expected %d)\n", len, (int)sizeof(TestPacket));
    return;
  }

  TestPacket pkt;
  memcpy(&pkt, data, sizeof(TestPacket));

  char senderStr[18];
  snprintf(senderStr, sizeof(senderStr), "%02X:%02X:%02X:%02X:%02X:%02X",
    info->src_addr[0], info->src_addr[1], info->src_addr[2],
    info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  Serial.printf("Received %s #%lu from %s\n", pkt.type, (unsigned long)pkt.counter, senderStr);

  // Reply with PONG if we received a PING
  if (strncmp(pkt.type, "PING", 4) == 0) {
    TestPacket reply;
    strncpy(reply.type, "PONG", 5);
    reply.counter = pkt.counter;
    WiFi.macAddress(reply.senderMAC);

    esp_err_t result = esp_now_send(broadcastMAC, (uint8_t*)&reply, sizeof(reply));
    if (result == ESP_OK) {
      Serial.printf("  → Sent PONG #%lu\n", (unsigned long)reply.counter);
    } else {
      Serial.printf("  → Failed to send PONG (error %d)\n", result);
    }

    // Blink LED on receive
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
  }
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(1500);  // Wait for USB CDC to connect
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Print own MAC so you can identify which board is which
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("ESP-NOW Test Ready. My MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed — check WiFi mode");
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceived);

  // Register broadcast peer
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
    return;
  }

  espnowReady = true;
  Serial.println("Broadcasting PING every 3 seconds. Waiting for peer...");
}

// ── Loop ──────────────────────────────────────────────────────────────────────

void loop() {
  if (!espnowReady) return;

  if (millis() - lastPingSent > 3000) {
    lastPingSent = millis();
    pingCounter++;

    TestPacket pkt;
    strncpy(pkt.type, "PING", 5);
    pkt.counter = pingCounter;
    WiFi.macAddress(pkt.senderMAC);

    esp_err_t result = esp_now_send(broadcastMAC, (uint8_t*)&pkt, sizeof(pkt));
    if (result == ESP_OK) {
      Serial.printf("Sent PING #%lu\n", (unsigned long)pingCounter);
    } else {
      Serial.printf("Failed to send PING #%lu (error %d)\n", (unsigned long)pingCounter, result);
    }

    // Blink LED on send
    digitalWrite(LED_PIN, HIGH);
    delay(20);
    digitalWrite(LED_PIN, LOW);
  }
}
