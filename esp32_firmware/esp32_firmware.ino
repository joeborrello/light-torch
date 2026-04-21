/*
 * ESP32 Firmware - Motion Capture Relay
 * Receives motion data from Proffie via UART, transmits via ESP-NOW
 * Receives motion data from peer ESP32, forwards to Proffie via UART
 */

#include <esp_now.h>
#include <WiFi.h>
#include "uart_handler.h"
#include "espnow_handler.h"

// Pin definitions for Arduino Nano ESP32
#define RX_PIN 0   // RX0 - Connected to Proffie TX
#define TX_PIN 1   // TX1 - Connected to Proffie RX
#define LED_PIN 13 // Built-in LED on Arduino Nano ESP32

// Global handlers
UARTHandler uartHandler;
ESPNowHandler espnowHandler;

// State machine
enum DeviceState {
  STATE_IDLE,
  STATE_RECEIVING_FROM_PROFFIE,
  STATE_TRANSMITTING_ESPNOW,
  STATE_RECEIVING_ESPNOW,
  STATE_FORWARDING_TO_PROFFIE
};

DeviceState currentState = STATE_IDLE;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize UART to Proffie
  uartHandler.begin(RX_PIN, TX_PIN);
  
  // Initialize ESP-NOW
  espnowHandler.begin();
  
  Serial.println("ESP32 Motion Relay Ready");
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // Check for incoming UART data from Proffie
  if (uartHandler.available()) {
    handleProffieData();
  }
  
  // Check for incoming ESP-NOW data from peer
  if (espnowHandler.hasReceivedData()) {
    handleESPNowData();
  }
  
  // Update status LED based on state
  updateStatusLED();
  
  delay(1);
}

void handleProffieData() {
  UARTPacket pkt;
  if (uartHandler.receivePacket(&pkt)) {
    switch (pkt.type) {
      case PKT_START_TRANSMIT:
        currentState = STATE_RECEIVING_FROM_PROFFIE;
        espnowHandler.startTransmission();
        uartHandler.sendAck();
        Serial.println("Started receiving from Proffie");
        break;
        
      case PKT_MOTION_DATA:
        if (currentState == STATE_RECEIVING_FROM_PROFFIE) {
          // Forward motion data via ESP-NOW
          espnowHandler.sendMotionPacket(pkt.payload, pkt.length);
          uartHandler.sendAck();
        }
        break;
        
      case PKT_END_TRANSMIT:
        if (currentState == STATE_RECEIVING_FROM_PROFFIE) {
          espnowHandler.endTransmission();
          currentState = STATE_IDLE;
          uartHandler.sendAck();
          Serial.println("Transmission complete");
        }
        break;
        
      default:
        Serial.printf("Unknown packet type: 0x%02X\n", pkt.type);
        break;
    }
  }
}

void handleESPNowData() {
  ESPNowPacket espPkt;
  if (espnowHandler.receivePacket(&espPkt)) {
    switch (espPkt.type) {
      case ESP_PKT_START:
        currentState = STATE_RECEIVING_ESPNOW;
        uartHandler.sendStartReceive(espPkt.totalSamples);
        Serial.printf("Started receiving %d samples via ESP-NOW\n", espPkt.totalSamples);
        break;
        
      case ESP_PKT_MOTION:
        if (currentState == STATE_RECEIVING_ESPNOW) {
          // Forward to Proffie via UART
          uartHandler.sendMotionData(espPkt.payload, espPkt.length);
        }
        break;
        
      case ESP_PKT_END:
        if (currentState == STATE_RECEIVING_ESPNOW) {
          uartHandler.sendEndReceive();
          currentState = STATE_IDLE;
          Serial.println("ESP-NOW reception complete");
        }
        break;
        
      default:
        Serial.printf("Unknown ESP-NOW packet type: 0x%02X\n", espPkt.type);
        break;
    }
  }
}

void updateStatusLED() {
  static unsigned long lastBlink = 0;
  unsigned long now = millis();
  
  switch (currentState) {
    case STATE_IDLE:
      // Slow blink (1 Hz)
      if (now - lastBlink > 500) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastBlink = now;
      }
      break;
      
    case STATE_RECEIVING_FROM_PROFFIE:
    case STATE_TRANSMITTING_ESPNOW:
      // Fast blink (5 Hz)
      if (now - lastBlink > 100) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastBlink = now;
      }
      break;
      
    case STATE_RECEIVING_ESPNOW:
    case STATE_FORWARDING_TO_PROFFIE:
      // Solid on
      digitalWrite(LED_PIN, HIGH);
      break;
  }
}
