#include "Command_Module.h"
#include "MAX30102_Module.h"
#include "PulseSensor_Module.h"
#include "DS18B20_Module.h"
#include "ECG_Module.h"
#include "GPS_Module.h"
#include "GSM_Module.h"
#include "Emergency.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

CommandHandler commandHandler;

bool CommandHandler::init() {
  _serialLen = 0;
  memset(_serialLine, 0, sizeof(_serialLine));
  DEBUG_PRINTLN(F("[CMD] Full-report ready. Type CHECK or STATUS."));
  return true;
}

void CommandHandler::update() {
  while (DEBUG_SERIAL.available() > 0) {
    const char c = (char)DEBUG_SERIAL.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      _serialLine[_serialLen] = '\0';
      if (_serialLen > 0) {
        normalizeCommand(_serialLine);
        if (isStatusCommand(_serialLine)) {
          handleCommand("SERIAL", EMERGENCY_PHONE_NUMBER);
        } else if (strcmp(_serialLine, "HELP") == 0) {
          DEBUG_PRINTLN(F("Commands: CHECK STATUS DATA ALL INFO REPORT HELP"));
          DEBUG_PRINTLN(F("Returns FULL data from ALL modules (Serial + multi-SMS)"));
        } else {
          DEBUG_PRINT(F("[CMD] Unknown: "));
          DEBUG_PRINTLN(_serialLine);
          DEBUG_PRINTLN(F("Type HELP"));
        }
      }
      _serialLen = 0;
      _serialLine[0] = '\0';
    } else if (_serialLen < (sizeof(_serialLine) - 1)) {
      _serialLine[_serialLen++] = c;
    }
  }

  char sender[24];
  char body[96];
  if (gsmModule.pollIncomingSms(sender, sizeof(sender), body, sizeof(body))) {
    normalizeCommand(body);
    DEBUG_PRINT(F("[CMD] SMS from "));
    DEBUG_PRINT(sender);
    DEBUG_PRINT(F(": "));
    DEBUG_PRINTLN(body);

    if (isStatusCommand(body)) {
      const char *replyTo = (sender[0] != '\0') ? sender : EMERGENCY_PHONE_NUMBER;
      handleCommand("SMS", replyTo);
    }
  }
}

void CommandHandler::normalizeCommand(char *text) const {
  size_t len = strlen(text);
  while (len > 0 && isspace((unsigned char)text[len - 1])) {
    text[--len] = '\0';
  }
  size_t start = 0;
  while (text[start] != '\0' && isspace((unsigned char)text[start])) {
    ++start;
  }
  if (start > 0) {
    memmove(text, text + start, strlen(text + start) + 1);
  }
  for (char *p = text; *p; ++p) {
    *p = (char)toupper((unsigned char)*p);
  }
}

bool CommandHandler::isStatusCommand(const char *text) const {
  if (text == NULL || text[0] == '\0') {
    return false;
  }
  if (strcmp(text, "STATUS") == 0 ||
      strcmp(text, "DATA") == 0 ||
      strcmp(text, "CHECK") == 0 ||
      strcmp(text, "ALL") == 0 ||
      strcmp(text, "INFO") == 0 ||
      strcmp(text, "REPORT") == 0 ||
      strcmp(text, "FULL") == 0) {
    return true;
  }
  return (strstr(text, "STATUS") != NULL) ||
         (strstr(text, "DATA") != NULL) ||
         (strstr(text, "CHECK") != NULL) ||
         (strstr(text, "REPORT") != NULL) ||
         (strstr(text, "FULL") != NULL);
}

void CommandHandler::handleCommand(const char *source, const char *replyPhone) {
  DEBUG_PRINT(F("[CMD] FULL ALL-MODULE report via "));
  DEBUG_PRINTLN(source);

  printFullReportToSerial();
  sendFullSmsReport(replyPhone);
}

void CommandHandler::printFullReportToSerial() const {
  const MAX30102Data    &maxd  = max30102.getData();
  const PulseSensorData &pulse = pulseSensor.getData();
  const DS18B20Data     &temp  = ds18b20.getData();
  const ECGData         &ecg   = ecgModule.getData();
  const GPSData         &gps   = gpsModule.getData();
  const GSMData         &gsm   = gsmModule.getData();
  const EmergencyStatus &em    = emergencyHandler.getStatus();

  DEBUG_PRINTLN();
  DEBUG_PRINTLN(F("######## FULL SYSTEM REPORT — ALL MODULES ########"));
  DEBUG_PRINTLN(F("--------------------------------------------------"));

  // 1) Temperature
  DEBUG_PRINTLN(F("[1] DS18B20 BODY TEMPERATURE"));
  DEBUG_PRINT(F("    Ready     : "));
  DEBUG_PRINTLN(temp.sensorReady ? F("YES") : F("NO / OFF"));
  DEBUG_PRINT(F("    Temp C    : "));
  DEBUG_PRINTLN(temp.temperatureC, 2);
  DEBUG_PRINT(F("    Temp F    : "));
  DEBUG_PRINTLN(C_TO_F(temp.temperatureC), 2);

  // 2) MAX30102
  DEBUG_PRINTLN(F("[2] MAX30102 HEART RATE / SpO2"));
  DEBUG_PRINT(F("    Ready     : "));
  DEBUG_PRINTLN(maxd.sensorReady ? F("YES") : F("NO / OFF"));
  DEBUG_PRINT(F("    Finger    : "));
  DEBUG_PRINTLN(maxd.fingerDetected ? F("YES") : F("NO"));
  DEBUG_PRINT(F("    HeartRate : "));
  DEBUG_PRINT(maxd.heartRateBpm);
  DEBUG_PRINTLN(F(" bpm"));
  DEBUG_PRINT(F("    SpO2      : "));
  DEBUG_PRINT(maxd.spo2Percent);
  DEBUG_PRINTLN(F(" %"));

  // 3) Pulse
  DEBUG_PRINTLN(F("[3] PULSE SENSOR (A1/GPIO34)"));
  DEBUG_PRINT(F("    Ready     : "));
  DEBUG_PRINTLN(pulse.sensorReady ? F("YES") : F("NO / OFF"));
  DEBUG_PRINT(F("    Finger    : "));
  DEBUG_PRINTLN(pulse.fingerDetected ? F("YES") : F("NO"));
  DEBUG_PRINT(F("    BPM       : "));
  DEBUG_PRINTLN(pulse.bpm);
  DEBUG_PRINT(F("    Raw ADC   : "));
  DEBUG_PRINTLN(pulse.rawValue);

  // 4) ECG
  DEBUG_PRINTLN(F("[4] ECG AD8232"));
  DEBUG_PRINT(F("    Ready     : "));
  DEBUG_PRINTLN(ecg.sensorReady ? F("YES") : F("NO / OFF"));
  DEBUG_PRINT(F("    Lead Off  : "));
  DEBUG_PRINTLN(ecg.leadOff ? F("YES (leads disconnected)") : F("NO (connected)"));
  DEBUG_PRINT(F("    Raw ADC   : "));
  DEBUG_PRINTLN(ecg.rawValue);
  DEBUG_PRINT(F("    Filtered  : "));
  DEBUG_PRINTLN(ecg.filteredValue);
  DEBUG_PRINT(F("    Baseline  : "));
  DEBUG_PRINTLN(ecg.baseline);

  // 5) GPS
  DEBUG_PRINTLN(F("[5] GPS NEO-6M"));
  DEBUG_PRINT(F("    Ready     : "));
  DEBUG_PRINTLN(gps.moduleReady ? F("YES") : F("NO / OFF"));
  DEBUG_PRINT(F("    Fix       : "));
  DEBUG_PRINTLN(gps.fixValid ? F("VALID") : F("NO FIX"));
  DEBUG_PRINT(F("    Satellites: "));
  DEBUG_PRINTLN(gps.satellites);
  DEBUG_PRINT(F("    Latitude  : "));
  DEBUG_PRINTLN(gps.latitude, 6);
  DEBUG_PRINT(F("    Longitude : "));
  DEBUG_PRINTLN(gps.longitude, 6);
  DEBUG_PRINT(F("    Altitude  : "));
  DEBUG_PRINT(gps.altitudeM, 1);
  DEBUG_PRINTLN(F(" m"));
  if (gps.fixValid) {
    DEBUG_PRINT(F("    Maps URL  : https://maps.google.com/?q="));
    DEBUG_PRINT(gps.latitude, 6);
    DEBUG_PRINT(F(","));
    DEBUG_PRINTLN(gps.longitude, 6);
  }

  // 6) GSM
  DEBUG_PRINTLN(F("[6] GSM SIM800L"));
  DEBUG_PRINT(F("    Ready     : "));
  DEBUG_PRINTLN(gsm.moduleReady ? F("YES") : F("NO / OFF"));
  DEBUG_PRINT(F("    Network   : "));
  switch (gsm.networkState) {
    case GSM_STATE_READY:   DEBUG_PRINTLN(F("READY (registered)")); break;
    case GSM_STATE_INIT:    DEBUG_PRINTLN(F("INIT (searching)")); break;
    case GSM_STATE_SENDING: DEBUG_PRINTLN(F("SENDING")); break;
    default:                DEBUG_PRINTLN(F("OFFLINE")); break;
  }
  DEBUG_PRINT(F("    Signal CSQ: "));
  DEBUG_PRINTLN(gsm.signalStrength);
  DEBUG_PRINT(F("    Last SMS  : "));
  DEBUG_PRINTLN(gsm.lastSmsSuccess ? F("SUCCESS") : F("FAIL / none"));
  DEBUG_PRINT(F("    Alert Num : "));
  DEBUG_PRINTLN(EMERGENCY_PHONE_NUMBER);

  // 7) Emergency
  DEBUG_PRINTLN(F("[7] EMERGENCY HANDLER"));
  DEBUG_PRINT(F("    Active    : "));
  DEBUG_PRINTLN(em.active ? F("YES — SOS") : F("NO"));
  DEBUG_PRINT(F("    Level     : "));
  DEBUG_PRINTLN((int)em.level);
  DEBUG_PRINT(F("    SMS Sent  : "));
  DEBUG_PRINTLN(em.smsSent ? F("YES") : F("NO"));
  DEBUG_PRINT(F("    Reason    : "));
  DEBUG_PRINTLN(em.reason[0] ? em.reason : "(none)");

  DEBUG_PRINTLN(F("##################################################"));
  DEBUG_PRINTLN();
}

void CommandHandler::sendFullSmsReport(const char *replyPhone) const {
  if (gsmModule.getData().networkState != GSM_STATE_READY) {
    DEBUG_PRINTLN(F("[CMD] GSM not ready — full report on Serial only."));
    return;
  }

  const MAX30102Data    &maxd  = max30102.getData();
  const PulseSensorData &pulse = pulseSensor.getData();
  const DS18B20Data     &temp  = ds18b20.getData();
  const ECGData         &ecg   = ecgModule.getData();
  const GPSData         &gps   = gpsModule.getData();
  const GSMData         &gsm   = gsmModule.getData();
  const EmergencyStatus &em    = emergencyHandler.getStatus();

  char msg[160];

  // SMS 1/4 — Temperature + MAX30102
  snprintf(msg, sizeof(msg),
           "1/4 VITALS Temp:%.1fC/%.1fF(%s) HR:%d SpO2:%d%% finger:%s MAX:%s",
           temp.temperatureC,
           C_TO_F(temp.temperatureC),
           temp.sensorReady ? "ON" : "OFF",
           maxd.heartRateBpm,
           maxd.spo2Percent,
           maxd.fingerDetected ? "Y" : "N",
           maxd.sensorReady ? "ON" : "OFF");
  DEBUG_PRINTLN(F("[CMD] Sending SMS 1/4..."));
  gsmModule.sendSMS(replyPhone, msg);
  delay(1500);

  // SMS 2/4 — Pulse + ECG
  snprintf(msg, sizeof(msg),
           "2/4 PulseBPM:%d raw:%d(%s) ECG:%d base:%d leadOff:%s(%s)",
           pulse.bpm,
           pulse.rawValue,
           pulse.sensorReady ? "ON" : "OFF",
           ecg.filteredValue,
           ecg.baseline,
           ecg.leadOff ? "Y" : "N",
           ecg.sensorReady ? "ON" : "OFF");
  DEBUG_PRINTLN(F("[CMD] Sending SMS 2/4..."));
  gsmModule.sendSMS(replyPhone, msg);
  delay(1500);

  // SMS 3/4 — GPS full
  if (gps.fixValid) {
    snprintf(msg, sizeof(msg),
             "3/4 GPS FIX sats:%d lat:%.6f lon:%.6f alt:%.1fm maps.google.com/?q=%.5f,%.5f",
             gps.satellites,
             gps.latitude, gps.longitude, gps.altitudeM,
             gps.latitude, gps.longitude);
  } else {
    snprintf(msg, sizeof(msg),
             "3/4 GPS NOFIX ready:%s sats:%d lat:%.6f lon:%.6f",
             gps.moduleReady ? "ON" : "OFF",
             gps.satellites,
             gps.latitude, gps.longitude);
  }
  DEBUG_PRINTLN(F("[CMD] Sending SMS 3/4..."));
  gsmModule.sendSMS(replyPhone, msg);
  delay(1500);

  // SMS 4/4 — GSM + Emergency
  const char *net =
      (gsm.networkState == GSM_STATE_READY)   ? "READY" :
      (gsm.networkState == GSM_STATE_INIT)    ? "INIT" :
      (gsm.networkState == GSM_STATE_SENDING) ? "SEND" : "OFF";
  snprintf(msg, sizeof(msg),
           "4/4 GSM:%s CSQ:%d SOS:%s lvl:%d reason:%s",
           net,
           gsm.signalStrength,
           em.active ? "YES" : "NO",
           (int)em.level,
           em.reason[0] ? em.reason : "none");
  DEBUG_PRINTLN(F("[CMD] Sending SMS 4/4..."));
  gsmModule.sendSMS(replyPhone, msg);

  DEBUG_PRINTLN(F("[CMD] Full 4-part SMS report finished."));
}
