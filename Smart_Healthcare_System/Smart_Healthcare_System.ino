/*
 * Smart Healthcare System — FULL MODE
 * Board: ESP32-WROOM-32 (also supports Arduino Mega 2560)
 *
 * Features:
 *   TFT dashboard, MAX30102, Pulse, MLX90614, ECG,
 *   GPS location, GSM SMS (emergency + periodic health report)
 */

#include "Config.h"
#include "TFT_Display.h"
#include "MAX30102_Module.h"
#include "PulseSensor_Module.h"
#include "MLX90614_Module.h"
#include "ECG_Module.h"
#include "GPS_Module.h"
#include "GSM_Module.h"
#include "Emergency.h"

static bool g_tftOk = false;

static void printPinMap();
static void initializeAllModules();
static void updateAllModules();
static void refreshDisplay();

void setup() {
  DEBUG_SERIAL.begin(DEBUG_BAUD_RATE);
  delay(1500);

  DEBUG_PRINTLN();
  DEBUG_PRINTLN(F("========================================"));
  DEBUG_PRINTLN(F("  SMART HEALTHCARE SYSTEM — FULL MODE"));
  DEBUG_PRINTLN(F("  Serial Monitor baud: 115200"));
  DEBUG_PRINTLN(F("========================================"));
  printPinMap();

#ifdef ESP32
  analogReadResolution(12);
  randomSeed(analogRead(PULSE_SENSOR_PIN));
#else
  randomSeed(analogRead(A15));
#endif

  initializeAllModules();

  if (g_tftOk) {
    tftDisplay.showMessage("FULL SYSTEM", "Monitoring + SMS active", ILI9341_GREEN);
  }
  DEBUG_PRINTLN(F("[System] Full monitoring started."));
  DEBUG_PRINT(F("[System] Emergency SMS -> "));
  DEBUG_PRINTLN(EMERGENCY_PHONE_NUMBER);
  DEBUG_PRINTLN(F("========================================"));
  delay(1200);
}

void loop() {
  updateAllModules();
  emergencyHandler.sendHealthReport();

  if (g_tftOk) {
    refreshDisplay();
  }

  delay(SYSTEM_UPDATE_INTERVAL_MS);
}

static void printPinMap() {
#ifdef ESP32
  DEBUG_PRINTLN(F("Board: ESP32-WROOM-32"));
  DEBUG_PRINTLN(F("--- PIN MAP ---"));
  DEBUG_PRINTLN(F("TFT   CS=5 DC=2 RST=4 MOSI=23 SCK=18"));
  DEBUG_PRINTLN(F("I2C1  MAX30102 SDA=21 SCL=22 INT=27"));
  DEBUG_PRINTLN(F("I2C2  MLX90614 SDA=25 SCL=26"));
  DEBUG_PRINTLN(F("Pulse GPIO34 | ECG GPIO35 LO+=32 LO-=33"));
  DEBUG_PRINTLN(F("GPS   RX=17 TX=16"));
  DEBUG_PRINTLN(F("GSM   RX=14 TX=15 (SIM800L external 4V)"));
#else
  DEBUG_PRINTLN(F("Board: Arduino Mega 2560"));
  DEBUG_PRINTLN(F("TFT CS=10 DC=8 RST=9 MOSI=51 SCK=52"));
#endif
  DEBUG_PRINTLN(F("----------------"));
}

static void initializeAllModules() {
  DEBUG_PRINTLN(F("[Init] Starting all modules..."));

#if ENABLE_TFT_MODULE
  g_tftOk = tftDisplay.init();
  if (!g_tftOk) {
    DEBUG_PRINTLN(F("[Init] TFT FAILED"));
  }
#endif

#if ENABLE_I2C_SENSORS
  if (!max30102.init()) {
    DEBUG_PRINTLN(F("[Init] MAX30102 FAILED — check I2C1 SDA=21 SCL=22"));
  }
  if (!mlx90614.init()) {
    DEBUG_PRINTLN(F("[Init] MLX90614 FAILED — check I2C2 SDA=25 SCL=26"));
  }
#endif

#if ENABLE_ANALOG_SENSORS
  if (!pulseSensor.init()) {
    DEBUG_PRINTLN(F("[Init] Pulse sensor FAILED"));
  }
  if (!ecgModule.init()) {
    DEBUG_PRINTLN(F("[Init] ECG FAILED"));
  }
#endif

#if ENABLE_GPS_MODULE
  if (!gpsModule.init()) {
    DEBUG_PRINTLN(F("[Init] GPS FAILED — check RX=17 TX=16"));
  }
#endif

#if ENABLE_GSM_MODULE
  if (!gsmModule.init()) {
    DEBUG_PRINTLN(F("[Init] GSM FAILED — check RX=14 TX=15 + 4V supply"));
  }
#endif

#if ENABLE_EMERGENCY_MODULE
  emergencyHandler.init();
#endif

  DEBUG_PRINTLN(F("[Init] Done."));
}

static void updateAllModules() {
#if ENABLE_I2C_SENSORS
  max30102.update();
  mlx90614.update();
#endif
#if ENABLE_ANALOG_SENSORS
  pulseSensor.update();
  ecgModule.update();
#endif
#if ENABLE_GPS_MODULE
  gpsModule.update();
#endif
#if ENABLE_GSM_MODULE
  gsmModule.update();
#endif
#if ENABLE_EMERGENCY_MODULE
  emergencyHandler.update();
#endif
}

static void refreshDisplay() {
  const MAX30102Data    &pulseOx = max30102.getData();
  const PulseSensorData &pulse   = pulseSensor.getData();
  const MLX90614Data    &temp    = mlx90614.getData();
  const ECGData         &ecg     = ecgModule.getData();
  const GPSData         &gps     = gpsModule.getData();
  const GSMData         &gsm     = gsmModule.getData();
  const EmergencyStatus &em      = emergencyHandler.getStatus();

  DisplayData d;
  d.bodyTemperatureC = temp.objectTemperatureC;
  d.heartRateBpm     = pulseOx.heartRateBpm;
  d.pulseSensorBpm   = pulse.bpm;
  d.spo2Percent      = pulseOx.spo2Percent;
  d.ecgValue         = ecg.filteredValue;
  d.ecgLeadOff       = ecg.leadOff;
  d.gpsFix           = gps.fixValid;
  d.latitude         = gps.latitude;
  d.longitude        = gps.longitude;
  d.gsmReady         = (gsm.networkState == GSM_STATE_READY);
  d.emergencyActive  = em.active;

  tftDisplay.update(d);
}
