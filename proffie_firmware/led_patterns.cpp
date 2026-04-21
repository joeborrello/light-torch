#include "led_patterns.h"
#include <math.h>

void LEDPatterns::begin(int pin, int numLeds) {
  strip = Adafruit_NeoPixel(numLeds, pin, NEO_GRBW + NEO_KHZ800);
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'
}

void LEDPatterns::updateFromMotion(const IMUData& data) {
  float intensity = calculateIntensity(data);
  uint32_t color = mapIntensityToColor(intensity);
  
  // Fill all LEDs with motion-mapped color
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
}

void LEDPatterns::setIdlePattern() {
  // Dim white breathing effect
  uint8_t brightness = (sin(millis() / 1000.0) + 1.0) * 32;  // 0-64 range
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, brightness));
  }
}

void LEDPatterns::setRecordingPattern() {
  // Pulsing red
  uint8_t brightness = (sin(millis() / 300.0) + 1.0) * 64;  // 0-128 range
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(brightness, 0, 0, 0));
  }
}

void LEDPatterns::setTransmittingPattern() {
  // Flashing blue
  uint8_t brightness = ((millis() / 200) % 2) ? 255 : 0;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, brightness, 0));
  }
}

void LEDPatterns::show() {
  strip.show();
}

uint32_t LEDPatterns::mapIntensityToColor(float intensity) {
  // Map 0.0-4.0g to color gradient: blue -> green -> yellow -> red
  intensity = constrain(intensity, 0.0, 4.0);
  uint8_t r, g, b, w;
  
  if (intensity < 1.0) {
    // Blue to cyan
    r = 0;
    g = (uint8_t)(intensity * 255);
    b = 255;
    w = 0;
  } else if (intensity < 2.0) {
    // Cyan to green
    r = 0;
    g = 255;
    b = (uint8_t)((2.0 - intensity) * 255);
    w = 0;
  } else if (intensity < 3.0) {
    // Green to yellow
    r = (uint8_t)((intensity - 2.0) * 255);
    g = 255;
    b = 0;
    w = 0;
  } else {
    // Yellow to red
    r = 255;
    g = (uint8_t)((4.0 - intensity) * 255);
    b = 0;
    w = 0;
  }
  
  return strip.Color(r, g, b, w);
}

float LEDPatterns::calculateIntensity(const IMUData& data) {
  // Calculate total acceleration magnitude
  float ax = data.ax / 8192.0;  // ±4g range
  float ay = data.ay / 8192.0;
  float az = data.az / 8192.0;
  return sqrt(ax*ax + ay*ay + az*az);
}

void LEDPatterns::setReceivingPattern() {
  // Purple/magenta breathing pattern to indicate receiving mode
  static uint8_t brightness = 0;
  static int8_t direction = 1;
  
  brightness += direction * 5;
  if (brightness >= 250 || brightness <= 5) {
    direction = -direction;
  }
  
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(brightness, 0, brightness, 0));
  }
  strip.show();
}

// ============================================================
// ERROR INDICATION PATTERNS
// ============================================================

void LEDPatterns::flashColor(uint32_t color, int count, int delayMs) {
  for (int flash = 0; flash < count; flash++) {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(delayMs);
    
    strip.clear();
    strip.show();
    delay(delayMs);
  }
}

void LEDPatterns::setErrorPattern_IMU() {
  // Solid RED - IMU initialization failed (critical hardware error)
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0, 0));  // Bright red
  }
}

void LEDPatterns::setErrorPattern_UARTTimeout() {
  // ORANGE pulse - UART communication timeout with ESP32
  uint8_t brightness = (sin(millis() / 200.0) + 1.0) * 127;
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(brightness, brightness/2, 0, 0));  // Orange
  }
}

void LEDPatterns::setErrorPattern_BufferOverflow() {
  // YELLOW fast blink - Motion buffer overflow (recording too long)
  uint8_t brightness = ((millis() / 150) % 2) ? 255 : 0;
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(brightness, brightness, 0, 0));  // Yellow
  }
}

void LEDPatterns::setErrorPattern_ESPNowFailed() {
  // MAGENTA flash - ESP-NOW peer not found or transmission failed
  uint8_t brightness = ((millis() / 300) % 2) ? 255 : 0;
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(brightness, 0, brightness, 0));  // Magenta
  }
}

void LEDPatterns::setErrorPattern_ChecksumFail() {
  // CYAN blink - Data corruption detected (checksum mismatch)
  uint8_t brightness = ((millis() / 250) % 2) ? 200 : 0;
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, brightness, brightness, 0));  // Cyan
  }
}
