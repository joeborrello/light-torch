/*
 * Motion Capture and Playback System - ProffieOS Config
 * 
 * This config integrates motion capture functionality into ProffieOS.
 * Hardware: Proffieboard V3.9 with built-in LSM6DS3 IMU
 * 
 * Features:
 * - Shake detection triggers 5-second motion recording
 * - Motion data transmitted to ESP32 via UART
 * - LED and audio feedback during recording/transmission
 * - Compatible with standard ProffieOS blade styles
 */

#ifdef CONFIG_TOP
#include "proffieboard_v3_config.h"

// ============================================================
// MOTION CAPTURE CONFIGURATION
// ============================================================
#define ENABLE_MOTION_CAPTURE
#define MOTION_RECORDING_DURATION_MS 5000
#define MOTION_SAMPLE_RATE_HZ 50
#define SHAKE_THRESHOLD_G 2.5
#define CLASH_THRESHOLD_G 3.0
#define UART_BAUD_RATE 115200

// UART pins for ESP32 communication
#define UART_TX_PIN 9   // PA9
#define UART_RX_PIN 10  // PA10

// ============================================================
// HARDWARE CONFIGURATION
// ============================================================
#define NUM_BLADES 1
#define NUM_BUTTONS 0
const unsigned int maxLedsPerStrip = 144;
#define EXTRA_COLOR_BUFFER_SPACE 60

// Enable I2C for built-in LSM6DS3 IMU
#define ENABLE_I2C

// V3 boards don't use serial flash, but ProffieOS expects this pin to be defined
#define serialFlashSelectPin 255
#define ENABLE_MOTION

// Enable SSD1306 OLED (optional - for status display)
// #define ENABLE_SSD1306

// Audio configuration
#define ENABLE_AUDIO
#define VOLUME 1500

// Power management
#define IDLE_OFF_TIME 60 * 15 * 1000  // 15 minutes
#define MOTION_TIMEOUT 60 * 15 * 1000

#endif

#ifdef CONFIG_PRESETS
// ============================================================
// BLADE STYLES
// ============================================================

// Globals from prop driving blade style layers
extern bool  g_await_pickup;
extern bool  g_sent_flash;
extern bool  g_awaiting_reply;
extern bool  g_sync_flash;
extern float g_playback_speed;
extern bool  g_in_playback;
extern float g_tilt_t;

class AwaitPickupF {
public:
  void run(BladeBase*) {}
  int getInteger(int)   { return g_await_pickup ? 32768 : 0; }
  int calculate(BladeBase*) { return g_await_pickup ? 32768 : 0; }
};

// Rapid white flash for 2s after transmission completes
class SentFlashF {
public:
  void run(BladeBase*) {}
  int getInteger(int)   { return g_sent_flash ? 32768 : 0; }
  int calculate(BladeBase*) { return g_sent_flash ? 32768 : 0; }
};

// Slow white pulse while awaiting a reply from the other board
class AwaitingReplyF {
public:
  void run(BladeBase*) {}
  int getInteger(int)   { return g_awaiting_reply ? 32768 : 0; }
  int calculate(BladeBase*) { return g_awaiting_reply ? 32768 : 0; }
};

// 500ms solid green flash when sync mode is toggled on or off
class SyncFlashF {
public:
  void run(BladeBase*) {}
  int getInteger(int)   { return g_sync_flash ? 32768 : 0; }
  int calculate(BladeBase*) { return g_sync_flash ? 32768 : 0; }
};

// Tilt position 0..1 → color mix 0..32768. In playback, uses stored speed instead.
class EffectiveTiltF {
public:
  void run(BladeBase*) {}
  int calculate(BladeBase*) {
    float t;
    if (g_in_playback) {
      t = g_playback_speed / 600.0f;
      if (t > 1.0f) t = 1.0f;
      if (t < 0.0f) t = 0.0f;
    } else {
      t = g_tilt_t;
    }
    int v = (int)(t * 32768.0f);
    return v > 32768 ? 32768 : (v < 0 ? 0 : v);
  }
  int getInteger(int) { return calculate(nullptr); }
};

// Tilt distance from home (0=home, 1=max tilt) → alpha 8192..32768 (25%→100%).
// In playback, uses stored speed as the distance proxy.
class TiltBrightnessF {
public:
  void run(BladeBase*) {}
  int calculate(BladeBase*) {
    float dist;
    if (g_in_playback) {
      dist = g_playback_speed / 600.0f;
    } else {
      dist = fabsf(g_tilt_t - 0.5f) * 2.0f;
    }
    if (dist > 1.0f) dist = 1.0f;
    if (dist < 0.0f) dist = 0.0f;
    return (int)(8192.0f + dist * 24576.0f);  // 25%..100%
  }
  int getInteger(int) { return calculate(nullptr); }
};

using MainStyle = Layers<
  // Standby base: warm white pulsing between full and ~half brightness
  Pulsing<Rgb<255,200,120>, Rgb<128,100,60>, 3000>,
  // Tilt color overlay: Red→Blue, brightness 25% at home → 100% at max tilt
  AlphaL<Mix<EffectiveTiltF, Red, Blue>, TiltBrightnessF>,
  // Slow white pulse while waiting for the other board to reply
  AlphaL<Pulsing<White, Black, 2000>, AwaitingReplyF>,
  // Rapidly pulsing red when awaiting pickup after data received
  AlphaL<Pulsing<Red, Black, 300>, AwaitPickupF>,
  // Rapid white flash for 2s immediately after transmission
  AlphaL<Pulsing<White, Black, 100>, SentFlashF>,
  // Solid green flash for 500ms when sync mode toggles on or off
  AlphaL<Green, SyncFlashF>,
  InOutTrL<TrWipe<300>, TrWipeIn<500>>
>;

// ============================================================
// PRESET CONFIGURATION
// ============================================================
Preset presets[] = {
  { "motion_capture", "tracks/",
    StylePtr<MainStyle>(),
    "motion_capture"
  }
};

BladeConfig blades[] = {
  { 0,
    WS281XBladePtr<144, bladePin, Color8::GRBW, PowerPINS<bladePowerPin2, bladePowerPin3>>(),
    CONFIGARRAY(presets)
  }
};

#endif

#ifdef CONFIG_BUTTONS
// No physical buttons on this build
#endif

#ifdef CONFIG_PROP
// ============================================================
// PROP FILE - Motion Capture Integration
// ============================================================
#include "../props/saber_motion_capture_prop.h"
#endif
