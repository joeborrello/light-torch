/*
 * Motion Capture Prop File for ProffieOS
 * Integrates motion recording, playback, and ESP32 communication
 */

#ifndef PROPS_SABER_MOTION_CAPTURE_PROP_H
#define PROPS_SABER_MOTION_CAPTURE_PROP_H

#include "prop_base.h"
#include "../common/fuse.h"

// Serial3 = LPUART1 = TX:PC1 (pin 17), RX:PC0 (pin 16) — the labeled TX/RX pads on Proffieboard V3.9

// Uncomment to enable UART diagnostic mode (periodic test packets, loopback test, verbose logging)
// Remove or comment out for production use
// #define UART_DIAGNOSTIC_MODE

// Accessed by blade style functors
bool g_await_pickup   = false;  // true while MC_AWAIT_PICKUP — drives pulsing red layer
bool g_sent_flash     = false;  // true for 2s after transmission — drives rapid white flash layer
bool g_awaiting_reply = false;  // true while MC_AWAITING_REPLY — drives slow white pulse layer
bool g_sync_flash     = false;  // true for 500ms when sync mode toggles — drives green flash layer
// Speed (deg/s) replayed from stored samples
float g_playback_speed = 0.0f;
bool  g_in_playback    = false;
bool  g_post_playback  = false;  // true after playback completes — drives fast-pulse "ready" standby
// Directional tilt color components 0..1: p1>0=left(Red), p1<0=right(Blue), p2>0=forward(Green), p2<0=backward(Yellow=R+G)
float g_tilt_color_r = 0.0f;
float g_tilt_color_g = 0.0f;
float g_tilt_color_b = 0.0f;
// Total 2D tilt distance from home 0..1 — drives brightness and volume
float g_tilt_magnitude = 0.0f;

// Motion capture states
enum MotionCaptureState {
  MC_IDLE,            // stationary, nothing pending
  MC_RECORDING,       // in motion, capturing samples
  MC_WAITING_DOCK,    // motion stopped, waiting for stability before transmit
  MC_TRANSMITTING,    // sending data to ESP32
  MC_RECEIVING,       // receiving data from peer ESP32
  MC_AWAIT_PICKUP,    // data received, showing pickup signal, waiting to be held
  MC_PLAYBACK,        // playing back received motion data
  MC_AWAITING_REPLY   // transmission sent, waiting for the other board to reply
};

// Motion data packet structure (matches ESP32 protocol)
struct MotionPacket {
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
  uint32_t timestamp;
};

#undef PROP_TYPE
#define PROP_TYPE MotionCaptureProp

class MotionCaptureProp : public PropBase {
public:
  MotionCaptureProp() : PropBase() {}

  const char* name() override { return "MotionCapture"; }

  static constexpr size_t   MOTION_BUFFER_SIZE   = 1500;           // 30s at 50Hz
  static constexpr uint32_t SAMPLE_INTERVAL_MS   = 20;             // 50Hz
  static constexpr uint32_t DOCK_SETTLE_MS        = 3000;           // stationary this long → transmit
  static constexpr uint32_t MOTION_STOP_MS        = 500;            // no motion this long → consider stopped
  static constexpr uint32_t MAX_RECORDING_MS      = 30000;          // safety cap
  static constexpr uint32_t REPLY_TIMEOUT_MS      = 3 * 60 * 1000; // 3 minutes → reset to IDLE
  static constexpr uint32_t MOTION_DEBOUNCE_MS    = 400;            // sustained motion required to trigger recording/playback
  static constexpr uint32_t SYNC_HOLD_MS           = 3000;           // sustained vigorous motion to toggle sync mode
  static constexpr uint32_t CALIBRATION_SETTLE_MS  = 5000;           // stationary this long (once at startup) → lock home orientation
  static constexpr float    ACCEL_FILTER_ALPHA      = 0.15f;          // low-pass weight for orientation smoothing
  static constexpr float    TILT_SENSITIVITY        = 0.4f;           // tilt magnitude (0..1) at which full response is reached (~24°)
  static constexpr float    STATIONARY_THRESHOLD   = 10.0f;          // deg/s — below this = stationary
  static constexpr float    MIN_SWING_SPEED        = 30.0f;          // deg/s — note generation threshold
  static constexpr float    RECORD_MIN_SPEED       = 20.0f;          // deg/s — recording/playback trigger threshold
  static constexpr float    SYNC_TRIGGER_SPEED     = 300.0f;         // deg/s — "blue light" threshold for sync mode toggle
  static constexpr float    MAX_SWING_SPEED        = 600.0f;

  MotionPacket motion_buffer[MOTION_BUFFER_SIZE];
  size_t motion_sample_count = 0;
  MotionCaptureState state = MC_IDLE;
  uint32_t recording_start_time = 0;
  uint32_t last_sample_time     = 0;
  uint32_t last_motion_time_    = 0;
  uint32_t dock_start_time_     = 0;
  uint32_t sent_flash_end_ms_   = 0;  // millis() at which g_sent_flash clears
  uint32_t sync_flash_end_ms_   = 0;  // millis() at which g_sync_flash clears
  uint32_t waiting_start_ms_    = 0;  // when MC_AWAIT_PICKUP or MC_AWAITING_REPLY was entered
  uint32_t first_motion_time_   = 0;  // debounce: when sustained motion above RECORD_MIN_SPEED began
  uint32_t sync_trigger_start_  = 0;  // when continuous vigorous motion began for sync toggle
  bool     sync_mode_           = false;
  bool     sync_trigger_active_ = false;

  // Orientation calibration
  Vec3     filtered_accel_   = {0.0f, 0.0f, 1.0f}; // low-pass filtered accelerometer
  Vec3     home_gravity_     = {0.0f, 0.0f, 1.0f}; // gravity direction at calibrated home position
  Vec3     tilt_axis_        = {1.0f, 0.0f, 0.0f}; // first axis perpendicular to home_gravity_ (drives Red↔Blue color)
  Vec3     tilt_axis2_       = {0.0f, 1.0f, 0.0f}; // second axis perpendicular to both (combined for magnitude)
  Vec3     cal_accel_sum_    = {0.0f, 0.0f, 0.0f}; // accumulator for calibration averaging
  int      cal_sample_count_ = 0;
  uint32_t idle_still_start_ = 0;                   // when continuous stillness in MC_IDLE began
  bool     calibrated_       = false;

  // UART communication (using Serial3 - TX=PC1, RX=PC0)
  void InitUART() {
#ifdef UART_DIAGNOSTIC_MODE
    uint32_t t = millis();
    while (!Serial && millis() - t < 3000) delay(10);
    STDOUT.println("InitUART: calling Serial3.begin...");
#endif
    Serial3.begin(115200);
#ifdef UART_DIAGNOSTIC_MODE
    STDOUT.println("InitUART: Serial3.begin returned");
    delay(100);
    uint8_t test[4] = {0x01, 0x02, 0x03, 0x04};
    size_t written = Serial3.write(test, 4);
    delay(5);
    STDOUT.print("Serial3 write test: ");
    STDOUT.print(written);
    STDOUT.println(" bytes written (expect 4)");
#endif
  }

#ifdef UART_DIAGNOSTIC_MODE
  uint32_t diag_last_send_ = 0;
  void SendDiagPacket() {
    uint8_t buf[70] = {};
    buf[0] = 0xAA;
    buf[1] = 0xFE;
    buf[2] = 5; buf[3] = 0;
    memcpy(buf + 4, "HELLO", 5);
    uint8_t cs = buf[1] ^ buf[2] ^ buf[3];
    for (int i = 0; i < 5; i++) cs ^= buf[4 + i];
    buf[68] = cs;
    buf[69] = 0x55;
    Serial3.write((uint8_t)0xFF);  delay(2);
    Serial3.write(buf,      32);   delay(5);
    Serial3.write(buf + 32, 32);   delay(5);
    Serial3.write(buf + 64,  6);   delay(5);
    STDOUT.println("Diag: sent test packet (71 bytes with dummy)");
  }
#endif

  void Setup() override {
    PropBase::Setup();
    InitUART();
    state             = MC_IDLE;
    motion_sample_count = 0;
    auto_on_done_     = false;
    filtered_accel_   = Vec3(0.0f, 0.0f, 1.0f);
    cal_accel_sum_    = Vec3(0.0f);
    cal_sample_count_ = 0;
    idle_still_start_ = 0;
    calibrated_       = false;
    STDOUT.println("Motion Capture Prop: Ready");
  }

  bool IsMoving() {
    return fusor.swing_speed() > MIN_SWING_SPEED;
  }

  bool IsStationary() {
    return fusor.swing_speed() < STATIONARY_THRESHOLD;
  }

  Vec3 NormVec3(const Vec3& v) {
    float l = v.len();
    return l > 0.0001f ? v * (1.0f / l) : Vec3(0.0f, 0.0f, 1.0f);
  }

  void UpdateFilteredAccel() {
    Vec3 a = fusor.accel();
    filtered_accel_ = filtered_accel_ * (1.0f - ACCEL_FILTER_ALPHA) + a * ACCEL_FILTER_ALPHA;
  }

  void CommitCalibration() {
    if (cal_sample_count_ == 0) return;
    home_gravity_ = NormVec3(cal_accel_sum_ * (1.0f / (float)cal_sample_count_));
    // Pick the world axis most perpendicular to home_gravity_ as the tilt reference direction
    float dx = fabsf(home_gravity_.x);
    float dy = fabsf(home_gravity_.y);
    float dz = fabsf(home_gravity_.z);
    Vec3 ref;
    if (dy <= dx && dy <= dz)      ref = Vec3(0.0f, 1.0f, 0.0f);
    else if (dz <= dx && dz <= dy) ref = Vec3(0.0f, 0.0f, 1.0f);
    else                           ref = Vec3(1.0f, 0.0f, 0.0f);
    // Gram-Schmidt: orthogonalize ref against home_gravity_
    tilt_axis_        = NormVec3(ref - home_gravity_ * home_gravity_.dot(ref));
    // Second tilt axis: perpendicular to both home_gravity_ and tilt_axis_
    tilt_axis2_       = NormVec3(home_gravity_.cross(tilt_axis_));
    calibrated_       = true;
    cal_accel_sum_    = Vec3(0.0f);
    cal_sample_count_ = 0;
    idle_still_start_ = 0;
    STDOUT.println("Orientation calibrated");
  }

  // Compute directional tilt color components and magnitude from current filtered accel.
  // tilt_axis_ positive = left (Red), negative = right (Blue)
  // tilt_axis2_ positive = forward (Green), negative = backward (Yellow = R+G)
  void UpdateTiltState() {
    if (!calibrated_) {
      g_tilt_color_r   = 0.0f;
      g_tilt_color_g   = 0.0f;
      g_tilt_color_b   = 0.0f;
      g_tilt_magnitude = 0.0f;
      return;
    }
    Vec3  gv = NormVec3(filtered_accel_);
    float p1 = gv.dot(tilt_axis_);   // +left / -right
    float p2 = gv.dot(tilt_axis2_);  // +forward / -backward
    float left_c     = p1 > 0.0f ? p1 / TILT_SENSITIVITY : 0.0f;
    float right_c    = p1 < 0.0f ? -p1 / TILT_SENSITIVITY : 0.0f;
    float forward_c  = p2 > 0.0f ? p2 / TILT_SENSITIVITY : 0.0f;
    float backward_c = p2 < 0.0f ? -p2 / TILT_SENSITIVITY : 0.0f;
    float r = left_c + backward_c;
    float g = forward_c + backward_c;
    float b = right_c;
    g_tilt_color_r   = r > 1.0f ? 1.0f : r;
    g_tilt_color_g   = g > 1.0f ? 1.0f : g;
    g_tilt_color_b   = b > 1.0f ? 1.0f : b;
    float m = sqrtf(p1 * p1 + p2 * p2) / TILT_SENSITIVITY;
    g_tilt_magnitude = m > 1.0f ? 1.0f : m;
  }

  // Allow starting from IDLE (normal) or PLAYBACK (auto-record response after playback)
  void StartRecording() {
    if (state != MC_IDLE) return;
    g_post_playback = false;
    state = MC_RECORDING;
    motion_sample_count  = 0;
    recording_start_time = millis();
    last_sample_time     = recording_start_time;
    last_motion_time_    = recording_start_time;
    STDOUT.println("Motion Capture: Recording started");
  }

  // Record a single motion sample
  void RecordSample() {
    if (state != MC_RECORDING) return;
    if (motion_sample_count >= MOTION_BUFFER_SIZE) return;

    uint32_t now = millis();
    if (now - last_sample_time < SAMPLE_INTERVAL_MS) return;

    Vec3 accel = fusor.accel();
    Vec3 gyro  = fusor.gyro();

    MotionPacket& packet = motion_buffer[motion_sample_count];
    packet.accel_x   = (int16_t)(accel.x * 100.0f);
    packet.accel_y   = (int16_t)(accel.y * 100.0f);
    packet.accel_z   = (int16_t)(accel.z * 100.0f);
    packet.gyro_x    = (int16_t)(gyro.x  * 100.0f);
    packet.gyro_y    = (int16_t)(gyro.y  * 100.0f);
    packet.gyro_z    = (int16_t)(gyro.z  * 100.0f);
    packet.timestamp = now - recording_start_time;

    motion_sample_count++;
    last_sample_time = now;
  }

  void StopRecording() {
    if (state != MC_RECORDING) return;
    STDOUT.print("Motion Capture: Stopped - ");
    STDOUT.print(motion_sample_count);
    STDOUT.println(" samples");
    if (motion_sample_count > 0) {
      dock_start_time_ = millis();
      state = MC_WAITING_DOCK;
      STDOUT.println("Motion Capture: Waiting for dock stability...");
    } else {
      state = MC_IDLE;
    }
  }

  // Transmit motion data via UART to ESP32
  void TransmitMotionData() {
    if (motion_sample_count == 0) {
      STDOUT.println("Motion Capture: No data to transmit");
      return;
    }

    state = MC_TRANSMITTING;
    STDOUT.println("Motion Capture: Transmitting to ESP32...");

    auto sendPkt = [&](uint8_t type, const uint8_t* data, uint16_t len) {
      uint8_t buf[70] = {};
      buf[0] = 0xAA;
      buf[1] = type;
      buf[2] = (uint8_t)(len & 0xFF);
      buf[3] = (uint8_t)(len >> 8);
      if (data && len) memcpy(buf + 4, data, len < 64 ? len : 64);
      uint8_t cs = buf[1] ^ buf[2] ^ buf[3];
      for (uint16_t i = 0; i < len && i < 64; i++) cs ^= buf[4 + i];
      buf[68] = cs;
      buf[69] = 0x55;
      Serial3.write((uint8_t)0xFF);  delay(2);
      Serial3.write(buf,      32);   delay(5);
      Serial3.write(buf + 32, 32);   delay(5);
      Serial3.write(buf + 64,  6);   delay(5);
      while (Serial3.available()) Serial3.read();
    };

    uint8_t startPayload[2] = {
      (uint8_t)(motion_sample_count & 0xFF),
      (uint8_t)(motion_sample_count >> 8)
    };
    sendPkt(0x01, startPayload, 2);

    uint8_t payload[56];
    for (size_t i = 0; i < motion_sample_count; i += 4) {
      uint16_t plen = 0;
      for (int j = 0; j < 4 && (i + j) < motion_sample_count; j++) {
        MotionPacket& p = motion_buffer[i + j];
        payload[plen++] = (uint8_t)(p.accel_x >> 8);   payload[plen++] = (uint8_t)(p.accel_x);
        payload[plen++] = (uint8_t)(p.accel_y >> 8);   payload[plen++] = (uint8_t)(p.accel_y);
        payload[plen++] = (uint8_t)(p.accel_z >> 8);   payload[plen++] = (uint8_t)(p.accel_z);
        payload[plen++] = (uint8_t)(p.gyro_x  >> 8);   payload[plen++] = (uint8_t)(p.gyro_x);
        payload[plen++] = (uint8_t)(p.gyro_y  >> 8);   payload[plen++] = (uint8_t)(p.gyro_y);
        payload[plen++] = (uint8_t)(p.gyro_z  >> 8);   payload[plen++] = (uint8_t)(p.gyro_z);
        payload[plen++] = (uint8_t)(p.timestamp >> 8); payload[plen++] = (uint8_t)(p.timestamp);
      }
      sendPkt(0x02, payload, plen);
    }

    sendPkt(0x03, nullptr, 0);

    STDOUT.print("Motion Capture: Transmitted ");
    STDOUT.print(motion_sample_count);
    STDOUT.println(" samples");

    sent_flash_end_ms_ = millis() + 2000;
    waiting_start_ms_  = millis();
    state = MC_AWAITING_REPLY;
  }

  bool auto_on_done_ = false;

  uint32_t last_note_time_ = 0;
  uint32_t arp_interval_   = 200;
  int      arp_step_       = 0;

  // Used in live mode and playback: tilt magnitude (0..1) drives pitch and arp rate
  void PlayNoteAtMagnitude(float magnitude) {
    static constexpr float pentatonic[20] = {
      130.81f, 146.83f, 164.81f, 196.00f, 220.00f,
      261.63f, 293.66f, 329.63f, 392.00f, 440.00f,
      523.25f, 587.33f, 659.25f, 783.99f, 880.00f,
     1046.50f,1174.66f,1318.51f,1567.98f,1760.00f
    };
    if (magnitude < 0.10f) { arp_step_ = 0; arp_interval_ = 200; return; }
    float curved = magnitude * magnitude;  // squaring spreads lower notes across more of the tilt range
    arp_interval_ = (uint32_t)(200.0f - curved * 150.0f);
    if (arp_interval_ < 50) arp_interval_ = 50;
    int note_idx = (int)(curved * 17.0f);
    if (note_idx > 17) note_idx = 17;
    if (note_idx < 0)  note_idx = 0;
    beeper.Beep(arp_interval_ / 1000.0f, pentatonic[note_idx + arp_step_]);
    arp_step_ = (arp_step_ + 1) % 3;
  }

  void PlayMotionNote() {
    PlayNoteAtMagnitude(g_tilt_magnitude);
  }

  // ── UART receive ──────────────────────────────────────────────
  uint8_t uart_rx_buf_[70] = {};
  uint8_t uart_rx_idx_     = 0;

  void PollUART() {
    while (Serial3.available()) {
      uint8_t b = (uint8_t)Serial3.read();
      if (uart_rx_idx_ == 0 && b != 0xAA) continue;
      uart_rx_buf_[uart_rx_idx_++] = b;
      if (uart_rx_idx_ == 70) {
        ProcessRxPacket();
        uart_rx_idx_ = 0;
      }
    }
  }

  void ProcessRxPacket() {
    if (uart_rx_buf_[0] != 0xAA || uart_rx_buf_[69] != 0x55) return;
    uint8_t         type    = uart_rx_buf_[1];
    uint16_t        len     = uart_rx_buf_[2] | ((uint16_t)uart_rx_buf_[3] << 8);
    const uint8_t*  payload = uart_rx_buf_ + 4;
    if (len > 64) return;

    switch (type) {
      case 0x07:  // PKT_START_RECEIVE
        motion_sample_count = 0;
        state = MC_RECEIVING;
        STDOUT.println("Receive: incoming motion data");
        break;

      case 0x02:  // PKT_MOTION_DATA
        if (state != MC_RECEIVING) break;
        for (uint16_t off = 0; off + 14 <= len && motion_sample_count < MOTION_BUFFER_SIZE; off += 14) {
          MotionPacket& p = motion_buffer[motion_sample_count++];
          p.accel_x   = (int16_t)((payload[off+ 0] << 8) | payload[off+ 1]);
          p.accel_y   = (int16_t)((payload[off+ 2] << 8) | payload[off+ 3]);
          p.accel_z   = (int16_t)((payload[off+ 4] << 8) | payload[off+ 5]);
          p.gyro_x    = (int16_t)((payload[off+ 6] << 8) | payload[off+ 7]);
          p.gyro_y    = (int16_t)((payload[off+ 8] << 8) | payload[off+ 9]);
          p.gyro_z    = (int16_t)((payload[off+10] << 8) | payload[off+11]);
          p.timestamp  = (uint32_t)((payload[off+12] << 8) | payload[off+13]);
        }
        break;

      case 0x08:  // PKT_END_RECEIVE
        if (state != MC_RECEIVING) break;
        STDOUT.print("Receive: ");
        STDOUT.print(motion_sample_count);
        STDOUT.println(" samples — pick me up to play back");
        playback_index_   = 0;
        waiting_start_ms_ = millis();
        g_await_pickup    = true;
        state = MC_AWAIT_PICKUP;
        break;
    }
  }

  // ── Playback ──────────────────────────────────────────────────
  size_t   playback_index_    = 0;
  uint32_t playback_start_ms_ = 0;

  void PlaybackLoop() {
    if (playback_index_ >= motion_sample_count) {
      STDOUT.println("Playback complete — ready to record");
      g_post_playback    = true;
      first_motion_time_ = 0;
      idle_still_start_  = 0;
      state = MC_IDLE;
      return;
    }

    MotionPacket& p = motion_buffer[playback_index_];
    if (millis() - playback_start_ms_ < (uint32_t)p.timestamp) return;

    // Compute tilt color and magnitude from stored accel using receiver's calibration axes
    if (calibrated_) {
      Vec3  stored_g = NormVec3(Vec3(p.accel_x / 100.0f, p.accel_y / 100.0f, p.accel_z / 100.0f));
      float p1 = stored_g.dot(tilt_axis_);
      float p2 = stored_g.dot(tilt_axis2_);
      float left_c     = p1 > 0.0f ? p1 / TILT_SENSITIVITY : 0.0f;
      float right_c    = p1 < 0.0f ? -p1 / TILT_SENSITIVITY : 0.0f;
      float forward_c  = p2 > 0.0f ? p2 / TILT_SENSITIVITY : 0.0f;
      float backward_c = p2 < 0.0f ? -p2 / TILT_SENSITIVITY : 0.0f;
      float r  = left_c + backward_c;
      float gc = forward_c + backward_c;
      float b  = right_c;
      g_tilt_color_r   = r  > 1.0f ? 1.0f : r;
      g_tilt_color_g   = gc > 1.0f ? 1.0f : gc;
      g_tilt_color_b   = b  > 1.0f ? 1.0f : b;
      float m = sqrtf(p1 * p1 + p2 * p2) / TILT_SENSITIVITY;
      g_tilt_magnitude = m > 1.0f ? 1.0f : m;

      // SD card: select swing file by direction from stored accel
      if (SFX_swing.files_found() > 0) {
        int   dir = 0;
        float mx  = 0.0f;
        if (p1  > mx) { mx = p1;  dir = 0; }
        if (-p1 > mx) { mx = -p1; dir = 1; }
        if (p2  > mx) { mx = p2;  dir = 2; }
        if (-p2 > mx) {            dir = 3; }
        int idx    = dir + (g_tilt_magnitude > 0.5f ? 4 : 0);
        int nfiles = (int)SFX_swing.files_found();
        if (idx >= nfiles) idx = dir < nfiles ? dir : 0;
        SFX_swing.Select(idx);
      }
    }

    // Beeper notes from stored tilt magnitude (SD card swing sounds can't be triggered during playback
    // because ProffieOS swing events require live IMU motion on the receiving device)
    uint32_t now = millis();
    if (SFX_swing.files_found() == 0 && now - last_note_time_ >= arp_interval_) {
      last_note_time_ = now;
      PlayNoteAtMagnitude(g_tilt_magnitude);
    }

    float gy = p.gyro_y / 100.0f;
    float gz = p.gyro_z / 100.0f;
    g_playback_speed = sqrtf(gy * gy + gz * gz);

    playback_index_++;
  }

  // Main loop - called continuously
  void Loop() override {
    PropBase::Loop();
    if (!auto_on_done_) {
      auto_on_done_ = true;
      On();
    }

    uint32_t now = millis();

    UpdateFilteredAccel();
    // During playback, tilt globals are set from stored accel in PlaybackLoop() instead
    if (state != MC_PLAYBACK) UpdateTiltState();

    // Volume: 25%→100% of VOLUME with tilt magnitude; full volume before calibration
    {
      float m = !calibrated_ ? 1.0f : g_tilt_magnitude;
      dynamic_mixer.set_volume((int32_t)(VOLUME * (0.25f + m * 0.75f)));
    }

    // Live notes: only when device is moving and no SD swing files present; suppressed during waiting states
    // (playback handles its own notes inside PlaybackLoop)
    if (SFX_swing.files_found() == 0 && !IsStationary() &&
        state != MC_PLAYBACK && state != MC_AWAIT_PICKUP && state != MC_AWAITING_REPLY) {
      if (now - last_note_time_ >= arp_interval_) {
        last_note_time_ = now;
        PlayMotionNote();
      }
    }

    // Blade-style globals
    g_in_playback    = (state == MC_PLAYBACK);
    g_sent_flash     = (now < sent_flash_end_ms_);
    g_awaiting_reply = (state == MC_AWAITING_REPLY && !g_sent_flash);

    // Sync mode toggle: 3 seconds of continuous motion above SYNC_TRIGGER_SPEED
    // Only allowed from idle-ish states, not mid-operation
    bool can_sync_toggle = (state == MC_IDLE || state == MC_AWAITING_REPLY);
    if (can_sync_toggle && fusor.swing_speed() > SYNC_TRIGGER_SPEED) {
      if (!sync_trigger_active_) {
        sync_trigger_active_ = true;
        sync_trigger_start_  = now;
      } else if (now - sync_trigger_start_ >= SYNC_HOLD_MS) {
        sync_mode_           = !sync_mode_;
        sync_trigger_active_ = false;
        g_await_pickup       = false;
        g_sent_flash         = false;
        g_awaiting_reply     = false;
        first_motion_time_   = 0;
        state                = MC_IDLE;
        idle_still_start_    = 0;
        sync_flash_end_ms_   = now + 500;
        STDOUT.println(sync_mode_ ? "Sync mode ON" : "Sync mode OFF");
      }
    } else {
      sync_trigger_active_ = false;
    }

    g_sync_flash = (now < sync_flash_end_ms_);

    // SD card swing sound: direction-mapped (playback handles its own selection inside PlaybackLoop)
    // 0=left(01), 1=right(02), 2=forward(03), 3=backward(04), +4 for 05-08 at high magnitude
    if (SFX_swing.files_found() > 0 && calibrated_ && state != MC_PLAYBACK) {
      Vec3  gv  = NormVec3(filtered_accel_);
      float p1  = gv.dot(tilt_axis_);
      float p2  = gv.dot(tilt_axis2_);
      int   dir = 0;
      float mx  = 0.0f;
      if (p1  > mx) { mx = p1;  dir = 0; }  // left
      if (-p1 > mx) { mx = -p1; dir = 1; }  // right
      if (p2  > mx) { mx = p2;  dir = 2; }  // forward
      if (-p2 > mx) { mx = -p2; dir = 3; }  // backward
      int idx    = dir + (g_tilt_magnitude > 0.5f ? 4 : 0);
      int nfiles = (int)SFX_swing.files_found();
      if (idx >= nfiles) idx = dir < nfiles ? dir : 0;
      SFX_swing.Select(idx);
    }

    if (state != MC_TRANSMITTING) PollUART();

    switch (state) {
      case MC_IDLE:
        // One-time startup calibration: find first 5-second window of stillness and lock home orientation
        if (!calibrated_) {
          if (IsStationary()) {
            if (idle_still_start_ == 0) {
              idle_still_start_ = now;
              cal_accel_sum_    = Vec3(0.0f);
              cal_sample_count_ = 0;
            }
            cal_accel_sum_ += fusor.accel();
            cal_sample_count_++;
            if (now - idle_still_start_ >= CALIBRATION_SETTLE_MS)
              CommitCalibration();  // sets calibrated_ = true, never runs again
          } else {
            idle_still_start_ = 0;
            cal_accel_sum_    = Vec3(0.0f);
            cal_sample_count_ = 0;
          }
        }

        if (!sync_mode_) {
          // Debounce: require RECORD_MIN_SPEED sustained for MOTION_DEBOUNCE_MS
          if (fusor.swing_speed() > RECORD_MIN_SPEED) {
            if (first_motion_time_ == 0) first_motion_time_ = now;
            else if (now - first_motion_time_ >= MOTION_DEBOUNCE_MS) {
              first_motion_time_ = 0;
              StartRecording();
            }
          } else {
            first_motion_time_ = 0;
          }
        }
#ifdef UART_DIAGNOSTIC_MODE
        if (millis() - diag_last_send_ > 5000) {
          diag_last_send_ = millis();
          SendDiagPacket();
        }
#endif
        break;

      case MC_RECORDING:
        RecordSample();
        if (IsMoving()) {
          last_motion_time_ = millis();
        } else if (millis() - last_motion_time_ > MOTION_STOP_MS) {
          StopRecording();
        }
        if (millis() - recording_start_time >= MAX_RECORDING_MS) StopRecording();
        break;

      case MC_WAITING_DOCK:
        if (IsMoving()) {
          STDOUT.println("Motion Capture: Picked up again — resuming recording");
          state = MC_RECORDING;
          last_motion_time_ = millis();
        } else if (millis() - dock_start_time_ >= DOCK_SETTLE_MS) {
          TransmitMotionData();
        }
        break;

      case MC_TRANSMITTING:
        break;

      case MC_RECEIVING:
        break;

      case MC_AWAIT_PICKUP:
        // Timeout: reset after 5 minutes
        if (millis() - waiting_start_ms_ >= REPLY_TIMEOUT_MS) {
          STDOUT.println("Await pickup timed out — resetting");
          g_await_pickup     = false;
          first_motion_time_ = 0;
          idle_still_start_  = 0;
          state = MC_IDLE;
          break;
        }
        // Debounce: require RECORD_MIN_SPEED sustained for MOTION_DEBOUNCE_MS to start playback
        if (fusor.swing_speed() > RECORD_MIN_SPEED) {
          if (first_motion_time_ == 0) first_motion_time_ = now;
          else if (now - first_motion_time_ >= MOTION_DEBOUNCE_MS) {
            STDOUT.println("Picked up — starting playback");
            g_await_pickup     = false;
            playback_start_ms_ = millis();
            first_motion_time_ = 0;
            state = MC_PLAYBACK;
          }
        } else {
          first_motion_time_ = 0;
        }
        break;

      case MC_PLAYBACK:
        PlaybackLoop();
        break;

      case MC_AWAITING_REPLY:
        if (millis() - waiting_start_ms_ >= REPLY_TIMEOUT_MS) {
          STDOUT.println("Awaiting reply timed out — resetting");
          g_sent_flash      = false;
          g_awaiting_reply  = false;
          idle_still_start_ = 0;
          state = MC_IDLE;
        }
        break;
    }
  }

  bool Event2(enum BUTTON button, EVENT event, uint32_t modifiers) override {
    return false;
  }
};

#endif // PROPS_SABER_MOTION_CAPTURE_PROP_H
