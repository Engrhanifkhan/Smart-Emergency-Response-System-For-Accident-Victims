#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===========================================================================
// Smart Healthcare System
// Mega 2560 functions → ESP32-WROOM pin map (dual I2C)
// ===========================================================================
//
//  Module            Mega 2560          ESP32-WROOM
//  ────────────────  ─────────────────  ──────────────────────────────
//  TFT CS            D10                GPIO 5   (your working LCD)
//  TFT DC            D8                 GPIO 2
//  TFT RST           D9                 GPIO 4
//  TFT MOSI (SDI)    D51                GPIO 23
//  TFT SCK           D52                GPIO 18
//  TFT MISO (SDO)    D50                GPIO 19 (optional)
//
//  MAX30102 SDA      D20 (I2C1)         GPIO 21  Wire  (I2C Bus 1)
//  MAX30102 SCL      D21 (I2C1)         GPIO 22  Wire
//  MAX30102 INT      D2                 GPIO 27
//
//  MLX90614 SDA      D20 shared Mega    GPIO 25  Wire1 (I2C Bus 2)
//  MLX90614 SCL      D21 shared Mega    GPIO 26  Wire1
//
//  Pulse Sensor      A1                 GPIO 34
//  ECG AOUT          A0                 GPIO 35
//  ECG LO+           D11                GPIO 32
//  ECG LO-           D12                GPIO 33
//
//  GPS RX (← GPS TX) D17 (RX2)          GPIO 17
//  GPS TX (→ GPS RX) D16 (TX2)          GPIO 16
//
//  GSM RX (← GSM TX) D19 (RX1)          GPIO 14  (avoid TFT SPI 18/19)
//  GSM TX (→ GSM RX) D18 (TX1)          GPIO 15
//
//  ESP32 note: Mega D8/D9/D10 cannot be reused (flash/strapping).
//  TFT keeps your tested CS=5 DC=2 RST=4. GSM moved off SPI pins.
// ===========================================================================

// --- Full system enable (ESP32-WROOM-32: ALL modules ON) -------------------
#ifdef ESP32
  #define LCD_TEST_ONLY             false
  #define ENABLE_TFT_MODULE         true
  #define ENABLE_GSM_MODULE         true
  #define ENABLE_GPS_MODULE         true
  #define ENABLE_I2C_SENSORS        true
  #define ENABLE_ANALOG_SENSORS     true
  #define ENABLE_EMERGENCY_MODULE   true
#else
  #define LCD_TEST_ONLY             true
  #define ENABLE_TFT_MODULE         true
  #define ENABLE_GSM_MODULE         false
  #define ENABLE_GPS_MODULE         false
  #define ENABLE_I2C_SENSORS        true
  #define ENABLE_ANALOG_SENSORS     true
  #define ENABLE_EMERGENCY_MODULE   true
#endif

// ADC mid-scale: Mega 10-bit (~512), ESP32 12-bit (~2048)
#ifdef ESP32
  #define ADC_MID_SCALE             2048
  #define PULSE_PEAK_THRESHOLD      2200
  #define PULSE_FINGER_MIN          2100
  #define PULSE_FINGER_MAX          4000
#else
  #define ADC_MID_SCALE             512
  #define PULSE_PEAK_THRESHOLD      550
  #define PULSE_FINGER_MIN          530
  #define PULSE_FINGER_MAX          1000
#endif

// --- System Timing Constants -----------------------------------------------
#define SYSTEM_UPDATE_INTERVAL_MS   20U    // Faster loop for smooth live ECG graph
#define SENSOR_SAMPLE_INTERVAL_MS   200U
#define DISPLAY_REFRESH_INTERVAL_MS 400U   // Temp/HR/SpO2 text refresh
#define GPS_UPDATE_INTERVAL_MS      1000U
#define PLOTTER_INTERVAL_MS         10U
#define GSM_HEARTBEAT_INTERVAL_MS   30000U

// --- Platform Pin Map ------------------------------------------------------
#ifdef ESP32  // ESP32-WROOM — Mega-equivalent functions + dual I2C

#define TFT_CS_PIN                  5
#define TFT_DC_PIN                  2
#define TFT_RST_PIN                 4
#define TFT_MOSI_PIN                23
#define TFT_SCK_PIN                 18
#define TFT_MISO_PIN                19
#define TFT_MIRROR_FIX              true
#define TFT_MADCTL_VALUE            0x88

// I2C Bus 1 — MAX30102 (Wire)  — same role as Mega D20/D21
#define I2C1_SDA_PIN                21
#define I2C1_SCL_PIN                22
#define MAX30102_INT_PIN            27

// I2C Bus 2 — MLX90614 (Wire1) — separate bus (Mega shared one I2C)
#define I2C2_SDA_PIN                25
#define I2C2_SCL_PIN                26

// Analog sensors — same roles as Mega A1 / A0 / D11 / D12
#define PULSE_SENSOR_PIN            34
#define ECG_ANALOG_PIN              35
#define ECG_LO_PLUS_PIN             32
#define ECG_LO_MINUS_PIN            33

// GPS — same UART pin numbers as Mega Serial2 (D16/D17)
#define GPS_SERIAL                  Serial2
#define GPS_BAUD_RATE               9600
#define GPS_RX_PIN                  17     // ESP RX ◄── GPS TX  (Mega D17)
#define GPS_TX_PIN                  16     // ESP TX ──► GPS RX  (Mega D16)

// GSM — same role as Mega Serial1; pins 14/15 (Mega used 19/18)
#define GSM_SERIAL                  Serial1
#define GSM_BAUD_RATE               9600
#define GSM_RX_PIN                  14     // ESP RX ◄── GSM TX  (Mega role D19)
#define GSM_TX_PIN                  15     // ESP TX ──► GSM RX  (Mega role D18)

#else // Arduino Mega 2560

#define TFT_CS_PIN                  10
#define TFT_DC_PIN                  8
#define TFT_RST_PIN                 9
#define TFT_MOSI_PIN                51
#define TFT_SCK_PIN                 52
#define TFT_MISO_PIN                50
#define TFT_SPI_SS_PIN              53
#define TFT_MIRROR_FIX              true
#define TFT_MADCTL_VALUE            0x88

#define I2C1_SDA_PIN                20
#define I2C1_SCL_PIN                21
#define MAX30102_INT_PIN            2

#define I2C2_SDA_PIN                12
#define I2C2_SCL_PIN                13

#define PULSE_SENSOR_PIN            A1
#define ECG_ANALOG_PIN              A0
#define ECG_LO_PLUS_PIN             11
#define ECG_LO_MINUS_PIN            12

#define GPS_SERIAL                  Serial2
#define GPS_BAUD_RATE               9600
#define GPS_RX_PIN                  17
#define GPS_TX_PIN                  16

#define GSM_SERIAL                  Serial1
#define GSM_BAUD_RATE               9600
#define GSM_RX_PIN                  19
#define GSM_TX_PIN                  18

#endif

#define I2C_MAX30102_ADDRESS        0x57
#define I2C_MLX90614_ADDRESS        0x5A
#define PULSE_SAMPLE_RATE_HZ        100
#define ECG_SAMPLE_RATE_HZ          100
#define ECG_BASELINE_SAMPLES        50
#define ECG_FILTER_ALPHA            0.35f

// --- Emergency SMS Settings ------------------------------------------------
#define EMERGENCY_PHONE_NUMBER      "+923118221686"
#define EMERGENCY_COOLDOWN_MS       60000UL
#define HEALTH_REPORT_SMS_INTERVAL_MS 120000UL

// --- Health Warning Thresholds ---------------------------------------------
#define TEMP_HIGH_THRESHOLD_C       38.5f
#define TEMP_LOW_THRESHOLD_C        35.0f
#define HR_HIGH_THRESHOLD_BPM       120
#define HR_LOW_THRESHOLD_BPM        50
#define SPO2_LOW_THRESHOLD_PCT      90

// --- Debugging via USB Serial ----------------------------------------------
#define DEBUG_SERIAL                Serial
#define DEBUG_BAUD_RATE             115200
#define ENABLE_DEBUG                true

#if ENABLE_DEBUG
  #define DEBUG_PRINT(x)            DEBUG_SERIAL.print(x)
  #define DEBUG_PRINTLN(x)          DEBUG_SERIAL.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#endif // CONFIG_H
