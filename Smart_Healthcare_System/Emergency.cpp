#include "Emergency.h"
#include <stdio.h>
#include <string.h>

EmergencyHandler emergencyHandler;

bool EmergencyHandler::init() {
  clear();
  DEBUG_PRINTLN(F("[Emergency] Handler initialized."));
  return true;
}

void EmergencyHandler::update() {
  const EmergencyLevel level = evaluateVitals();

  if (level == EMERGENCY_NONE) {
    if (_status.active) {
      _status.active = false;
      _status.level  = EMERGENCY_NONE;
    }
    return;
  }

  const unsigned long now = millis();
  if (_status.active && (now - _lastAlertMs) < EMERGENCY_COOLDOWN_MS) {
    return;
  }

  const char *reason = _status.reason;
  if (level == EMERGENCY_CRITICAL) {
    reason = "Critical vital signs";
  } else if (level == EMERGENCY_WARNING) {
    reason = "Warning: abnormal vitals";
  }

  dispatchAlert(level, reason);
}

void EmergencyHandler::clear() {
  _status.level     = EMERGENCY_NONE;
  _status.active      = false;
  _status.smsSent     = false;
  _status.reason[0]   = '\0';
  _lastAlertMs        = 0;
  _lastHealthReportMs = 0;
}

const EmergencyStatus &EmergencyHandler::getStatus() const {
  return _status;
}

void EmergencyHandler::triggerManualEmergency() {
  strncpy(_status.reason, "Manual SOS triggered", sizeof(_status.reason) - 1);
  dispatchAlert(EMERGENCY_CRITICAL, _status.reason);
}

EmergencyLevel EmergencyHandler::evaluateVitals() {
  const MAX30102Data    &pulseOx = max30102.getData();
  const PulseSensorData &pulse   = pulseSensor.getData();
  const DS18B20Data     &temp    = ds18b20.getData();
  const ECGData         &ecg     = ecgModule.getData();

  if (temp.temperatureC >= TEMP_HIGH_THRESHOLD_C) {
    strncpy(_status.reason, "High body temperature", sizeof(_status.reason) - 1);
    return EMERGENCY_CRITICAL;
  }

  if (temp.temperatureC > 0.0f &&
      temp.temperatureC <= TEMP_LOW_THRESHOLD_C) {
    strncpy(_status.reason, "Low body temperature", sizeof(_status.reason) - 1);
    return EMERGENCY_WARNING;
  }

  const int hrMax30102 = pulseOx.fingerDetected ? pulseOx.heartRateBpm : 0;
  const int hrPulse    = pulse.fingerDetected ? pulse.bpm : 0;
  const int heartRate  = (hrMax30102 > 0) ? hrMax30102 : hrPulse;

  if (heartRate > 0) {
    if (heartRate >= HR_HIGH_THRESHOLD_BPM || heartRate <= HR_LOW_THRESHOLD_BPM) {
      strncpy(_status.reason, "Abnormal heart rate", sizeof(_status.reason) - 1);
      return EMERGENCY_CRITICAL;
    }
  }

  if (pulseOx.fingerDetected &&
      pulseOx.spo2Percent > 0 &&
      pulseOx.spo2Percent < SPO2_LOW_THRESHOLD_PCT) {
    strncpy(_status.reason, "Low blood oxygen", sizeof(_status.reason) - 1);
    return EMERGENCY_CRITICAL;
  }

  if (ecg.sensorReady && ecg.leadOff) {
    strncpy(_status.reason, "ECG lead off", sizeof(_status.reason) - 1);
    return EMERGENCY_WARNING;
  }

  return EMERGENCY_NONE;
}

void EmergencyHandler::dispatchAlert(EmergencyLevel level, const char *reason) {
  const unsigned long now = millis();

  _status.level  = level;
  _status.active = (level != EMERGENCY_NONE);

  char message[160];
  buildVitalsMessage(message, sizeof(message), "SOS", reason);

  DEBUG_PRINT(F("[Emergency] Alert: "));
  DEBUG_PRINTLN(message);

  if (gsmModule.getData().networkState == GSM_STATE_READY) {
    _status.smsSent = gsmModule.sendSMS(EMERGENCY_PHONE_NUMBER, message);
    _lastAlertMs = now;
  } else {
    DEBUG_PRINTLN(F("[Emergency] GSM not ready — SMS skipped."));
  }
}

void EmergencyHandler::sendHealthReport() {
#if HEALTH_REPORT_SMS_INTERVAL_MS == 0
  return;
#else
  const unsigned long now = millis();
  if ((now - _lastHealthReportMs) < HEALTH_REPORT_SMS_INTERVAL_MS) {
    return;
  }

  if (gsmModule.getData().networkState != GSM_STATE_READY) {
    return;
  }

  char message[160];
  buildVitalsMessage(message, sizeof(message), "Health", NULL);

  DEBUG_PRINT(F("[Health] Report SMS: "));
  DEBUG_PRINTLN(message);

  if (gsmModule.sendSMS(EMERGENCY_PHONE_NUMBER, message)) {
    _lastHealthReportMs = now;
  }
#endif
}

void EmergencyHandler::buildVitalsMessage(char *buffer, size_t bufferSize,
                                          const char *prefix, const char *reason) {
  const MAX30102Data    &pulseOx = max30102.getData();
  const PulseSensorData &pulse   = pulseSensor.getData();
  const DS18B20Data     &temp    = ds18b20.getData();
  const ECGData         &ecg     = ecgModule.getData();
  const GPSData         &gps     = gpsModule.getData();

  const int hrMax   = pulseOx.fingerDetected ? pulseOx.heartRateBpm : 0;
  const int hrPulse = pulse.fingerDetected ? pulse.bpm : 0;
  const int hr      = (hrMax > 0) ? hrMax : hrPulse;

  if (reason != NULL && reason[0] != '\0') {
    if (gps.fixValid) {
      snprintf(buffer, bufferSize,
               "%s:%s T:%.1fC/%.1fF HR:%d P:%d SpO2:%d ECG:%d Loc:%.5f,%.5f",
               prefix, reason,
               temp.temperatureC, C_TO_F(temp.temperatureC),
               hr, hrPulse, pulseOx.spo2Percent,
               ecg.filteredValue, gps.latitude, gps.longitude);
    } else {
      snprintf(buffer, bufferSize,
               "%s:%s T:%.1fC/%.1fF HR:%d P:%d SpO2:%d ECG:%d Loc:N/A",
               prefix, reason,
               temp.temperatureC, C_TO_F(temp.temperatureC),
               hr, hrPulse, pulseOx.spo2Percent,
               ecg.filteredValue);
    }
    return;
  }

  if (gps.fixValid) {
    snprintf(buffer, bufferSize,
             "%s T:%.1fC/%.1fF HR:%d P:%d SpO2:%d ECG:%d Loc:%.5f,%.5f",
             prefix,
             temp.temperatureC, C_TO_F(temp.temperatureC),
             hr, hrPulse, pulseOx.spo2Percent,
             ecg.filteredValue, gps.latitude, gps.longitude);
  } else {
    snprintf(buffer, bufferSize,
             "%s T:%.1fC/%.1fF HR:%d P:%d SpO2:%d ECG:%d Loc:N/A",
             prefix,
             temp.temperatureC, C_TO_F(temp.temperatureC),
             hr, hrPulse, pulseOx.spo2Percent,
             ecg.filteredValue);
  }
}
