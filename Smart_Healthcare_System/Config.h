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
//  TFT HX8357D       480x320 landscape  (MADCTL 0xE8 mirror fix)
//  TFT CS            D10                GPIO 5   (confirmed working)
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
//  DS18B20 (temp)    OneWire             GPIO 25  (+ 4.7k to 3.3V)
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

// --- Module enables (use 1/0 only — ESP32 #if does NOT accept true/false) ---
#ifdef ESP32
  // 0 = full system (display always on; sensors optional)
  #define LCD_TEST_ONLY             0
  #define ENABLE_TFT_MODULE         1
  #define ENABLE_GSM_MODULE         1
  #define ENABLE_GPS_MODULE         1
  #define ENABLE_I2C_SENSORS        1
  #define ENABLE_ANALOG_SENSORS     1
  #define ENABLE_EMERGENCY_MODULE   1
#else
  #define LCD_TEST_ONLY             0
  #define ENABLE_TFT_MODULE         1
  #define ENABLE_GSM_MODULE         0
  #define ENABLE_GPS_MODULE         0
  #define ENABLE_I2C_SENSORS        1
  #define ENABLE_ANALOG_SENSORS     1
  #define ENABLE_EMERGENCY_MODULE   1
#endif

// ADC mid-scale: Mega 10-bit (~512), ESP32 12-bit (~2048)
#ifdef ESP32
  #define ADC_MID_SCALE             2048
#else
  #define ADC_MID_SCALE             512
#endif
// PulseSensor Amped uses adaptive baseline (no fixed finger min/max)

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
#define TFT_MIRROR_FIX              1
#define TFT_MADCTL_VALUE            0xE8   // confirmed on HX8357D

// I2C Bus 1 — MAX30102 (Wire)  — same role as Mega D20/D21
#define I2C1_SDA_PIN                21
#define I2C1_SCL_PIN                22
#define MAX30102_INT_PIN            27

// DS18B20 body temperature (OneWire) — was MLX90614 I2C pin 25
#define DS18B20_PIN                 25

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
#define TFT_MIRROR_FIX              1
#define TFT_MADCTL_VALUE            0xE8

#define I2C1_SDA_PIN                20
#define I2C1_SCL_PIN                21
#define MAX30102_INT_PIN            2

#define DS18B20_PIN                 4      // OneWire data (Mega)

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
#define PULSE_SAMPLE_RATE_HZ        100
#define ECG_SAMPLE_RATE_HZ          100
#define ECG_BASELINE_SAMPLES        50
#define ECG_FILTER_ALPHA            0.35f

// --- Emergency SMS Settings ------------------------------------------------
#define EMERGENCY_PHONE_NUMBER      "+923159915537"
#define EMERGENCY_COOLDOWN_MS       60000UL
#define HEALTH_REPORT_SMS_INTERVAL_MS 120000UL

// --- Remote status commands (Serial Monitor + GSM SMS) ---------------------
// Type in Serial: STATUS | DATA | CHECK | ALL | INFO | HELP
// SMS same words to the SIM → reply SMS with all sensor/GPS/GSM data
#define ENABLE_COMMAND_MODULE       1

// --- Human body monitoring ranges (shown on LCD) ---------------------------
// DS18B20 body temperature
#define TEMP_MIN_C                  35.0f   // hypothermia alert below
#define TEMP_MAX_C                  38.5f   // fever alert above
#define TEMP_NORMAL_MIN_C           36.1f
#define TEMP_NORMAL_MAX_C           37.2f
#define C_TO_F(c)                   (((c) * 9.0f / 5.0f) + 32.0f)

// MAX30102 SpO2 %
#define SPO2_MIN_PCT                95      // normal low
#define SPO2_MAX_PCT                100     // normal high
#define SPO2_LOW_THRESHOLD_PCT      90      // critical alert below

// PulseSensor Amped / MAX30102 heart rate (bpm)
#define PULSE_MIN_BPM               60      // normal adult low
#define PULSE_MAX_BPM               100     // normal adult high
#define HR_LOW_THRESHOLD_BPM        50      // bradycardia alert
#define HR_HIGH_THRESHOLD_BPM       120     // tachycardia alert

// AD8232 ECG — adult rate band + ADC plot window around baseline
#define ECG_MIN_BPM                 60
#define ECG_MAX_BPM                 100
#define ECG_ADC_SPAN                400     // ±span ADC counts on graph

// Legacy aliases used by Emergency module
#define TEMP_HIGH_THRESHOLD_C       TEMP_MAX_C
#define TEMP_LOW_THRESHOLD_C        TEMP_MIN_C

// Project name (boot splash)
#define PROJECT_NAME_LINE1          "Smart Emergency Response System"
#define PROJECT_NAME_LINE2          "For Accident Victims"
#define PROJECT_NAME_LINE3          "Vital Sign Monitoring"
#define PROJECT_NAME_LINE4          "And Data Transmission"

// --- Debugging via USB Serial ----------------------------------------------
#define DEBUG_SERIAL                Serial
#define DEBUG_BAUD_RATE             115200
#define ENABLE_DEBUG                1

#if ENABLE_DEBUG
  #define DEBUG_PRINT(...)            DEBUG_SERIAL.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...)          DEBUG_SERIAL.println(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
#endif

#endif // CONFIG_H
