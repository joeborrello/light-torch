#ifndef CONFIG_H
#define CONFIG_H

// Hardware pins
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 13
#define LED_DATA_PIN 2  // Data1 pad on Proffie V3.9 (use 3 for Data2)
#define UART_TX_PIN 9   // PA9 (TX pad)
#define UART_RX_PIN 10  // PA10 (RX pad)

// IMU settings
#define IMU_SAMPLE_RATE_HZ 50
#define SHAKE_THRESHOLD_G 2.5f
#define SHAKE_COOLDOWN_MS 1000

// Buffer settings
#define RECORDING_DURATION_SEC 10
#define MAX_SAMPLES (IMU_SAMPLE_RATE_HZ * RECORDING_DURATION_SEC)

// LED settings
#define NUM_LEDS 60
#define LED_BRIGHTNESS 128  // 0-255

// UART settings
#define UART_BAUD 115200
#define UART_TIMEOUT_MS 1000

// ESP32 peer MAC address (update after pairing)
#define ESP32_PEER_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}  // Broadcast

#endif // CONFIG_H
