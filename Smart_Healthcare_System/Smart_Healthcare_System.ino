/*
 * Smart Healthcare System — FULL MODE
 * Board: ESP32-WROOM-32
 *
 * Display ALWAYS starts and stays on.
 * Any missing sensor shows OFF / "--" — does not blank the screen.
 * Connected modules update live values.
 */

#include "Config.h"
#include "TFT_Display.h"
#include "MAX30102_Module.h"
#include "PulseSensor_Module.h"
#include "DS18B20_Module.h"
#include "ECG_Module.h"
#include "GPS_Module.h"
#include "GSM_Module.h"
#include "Emergency.h"
#include "Command_Module.h"
#include <string.h>

static bool g_tftOk      = false;
static bool g_max30102Ok = false;
static bool g_tempOk     = false;
static bool g_pulseOk    = false;
static bool g_ecgOk      = false;
static bool g_gpsOk      = false;
static bool g_gsmOk      = false;

static void printPinMap();
static void initializeAllModules();
static void updateAllModules();
static void refreshDisplay();

void setup() {
  Serial.begin(115200);
  delay(500);

  DEBUG_PRINTLN();
  DEBUG_PRINTLN(F("========================================"));
  DEBUG_PRINTLN(F("  Smart Emergency Response System"));
  DEBUG_PRINTLN(F("  For Accident Victims — Vital Signs"));
  DEBUG_PRINTLN(F("========================================"));
  printPinMap();

#ifdef ESP32
  analogReadResolution(12);
  randomSeed(analogRead(PULSE_SENSOR_PIN));
#endif

  // 1) Display first — never blocked by sensors
  initializeAllModules();

  DEBUG_PRINTLN(F("[System] Running. Missing modules stay OFF on LCD."));
  DEBUG_PRINT(F("[System] SMS phone: "));
  DEBUG_PRINTLN(EMERGENCY_PHONE_NUMBER);
}

void loop() {
  updateAllModules();
  commandHandler.update();

#if ENABLE_EMERGENCY_MODULE
  if (g_gsmOk) {
    emergencyHandler.sendHealthReport();
  }
#endif

  if (g_tftOk) {
    refreshDisplay();
  }

  delay(SYSTEM_UPDATE_INTERVAL_MS);
}

static void printPinMap() {
  DEBUG_PRINTLN(F("Board: ESP32-WROOM-32"));
  DEBUG_PRINTLN(F("TFT  HX8357D CS=5 DC=2 RST=4 MADCTL=0xE8"));
  DEBUG_PRINTLN(F("I2C1 MAX30102 SDA=21 SCL=22"));
  DEBUG_PRINTLN(F("DS18B20 temp OneWire GPIO25 (+4.7k pull-up)"));
  DEBUG_PRINTLN(F("Pulse GPIO34 | ECG GPIO35"));
  DEBUG_PRINTLN(F("GPS RX=17 TX=16 | GSM RX=14 TX=15"));
}

static void initializeAllModules() {
  DEBUG_PRINTLN(F("[Init] TFT first..."));

#if ENABLE_TFT_MODULE
  g_tftOk = tftDisplay.init();
  if (!g_tftOk) {
    DEBUG_PRINTLN(F("[Init] TFT FAILED"));
  } else {
    DEBUG_PRINTLN(F("[Init] TFT OK — dashboard live"));
  }
#endif

  // Sensors: fail quietly — display stays up
#if ENABLE_I2C_SENSORS
  g_max30102Ok = max30102.init();
  DEBUG_PRINTLN(g_max30102Ok ? F("[Init] MAX30102 OK") : F("[Init] MAX30102 OFF"));

  g_tempOk = ds18b20.init();
  DEBUG_PRINTLN(g_tempOk ? F("[Init] DS18B20 OK") : F("[Init] DS18B20 OFF"));
#endif

#if ENABLE_ANALOG_SENSORS
  g_pulseOk = pulseSensor.init();
  DEBUG_PRINTLN(g_pulseOk ? F("[Init] Pulse OK") : F("[Init] Pulse OFF"));

  g_ecgOk = ecgModule.init();
  DEBUG_PRINTLN(g_ecgOk ? F("[Init] ECG OK") : F("[Init] ECG OFF"));
#endif

#if ENABLE_GPS_MODULE
  g_gpsOk = gpsModule.init();
  DEBUG_PRINTLN(g_gpsOk ? F("[Init] GPS OK") : F("[Init] GPS OFF"));
#endif

#if ENABLE_GSM_MODULE
  g_gsmOk = gsmModule.init();
  DEBUG_PRINTLN(g_gsmOk ? F("[Init] GSM OK") : F("[Init] GSM OFF (no modem)"));
#endif

#if ENABLE_EMERGENCY_MODULE
  emergencyHandler.init();
#endif

  commandHandler.init();

  DEBUG_PRINTLN(F("[Init] Done — display keeps running."));
  DEBUG_PRINTLN(F(">>> Type STATUS in Serial Monitor for all sensor data"));
  DEBUG_PRINTLN(F(">>> Or SMS the word STATUS to this SIM card"));
}

static void updateAllModules() {
#if ENABLE_I2C_SENSORS
  if (g_max30102Ok) max30102.update();
  if (g_tempOk)     ds18b20.update();
#endif
#if ENABLE_ANALOG_SENSORS
  if (g_pulseOk) pulseSensor.update();
  if (g_ecgOk)   ecgModule.update();
#endif
#if ENABLE_GPS_MODULE
  if (g_gpsOk) gpsModule.update();
#endif
#if ENABLE_GSM_MODULE
  if (g_gsmOk) gsmModule.update();
#endif
#if ENABLE_EMERGENCY_MODULE
  if (g_max30102Ok || g_tempOk || g_pulseOk || g_ecgOk) {
    emergencyHandler.update();
  }
#endif
}

static void refreshDisplay() {
  DisplayData d;
  memset(&d, 0, sizeof(d));

  d.tempOk     = g_tempOk;
  d.max30102Ok = g_max30102Ok;
  d.pulseOk    = g_pulseOk;
  d.ecgOk      = g_ecgOk;
  d.gpsOk      = g_gpsOk;
  d.gsmOk      = g_gsmOk;

  if (g_tempOk) {
    d.bodyTemperatureC = ds18b20.getData().temperatureC;
  }

  if (g_max30102Ok) {
    const MAX30102Data &pulseOx = max30102.getData();
    d.heartRateBpm = pulseOx.heartRateBpm;
    d.spo2Percent  = pulseOx.spo2Percent;
  }

  if (g_pulseOk) {
    const PulseSensorData &pulse = pulseSensor.getData();
    d.pulseSensorBpm = pulse.bpm;
    d.pulseRaw       = pulse.rawValue;
  }

  if (g_ecgOk) {
    const ECGData &ecg = ecgModule.getData();
    d.ecgValue     = ecg.filteredValue;
    d.ecgBaseline  = ecg.baseline;
    d.ecgLeadOff   = ecg.leadOff;
  } else {
    d.ecgLeadOff = true;
  }

  if (g_gpsOk) {
    const GPSData &gps = gpsModule.getData();
    d.gpsFix    = gps.fixValid;
    d.latitude  = gps.latitude;
    d.longitude = gps.longitude;
  }

  if (g_gsmOk) {
    const GSMData &gsm = gsmModule.getData();
    d.gsmReady = (gsm.networkState == GSM_STATE_READY);
  }

#if ENABLE_EMERGENCY_MODULE
  d.emergencyActive = emergencyHandler.getStatus().active;
#endif

  tftDisplay.update(d);
}
