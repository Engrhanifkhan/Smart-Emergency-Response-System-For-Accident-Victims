#ifndef EMERGENCY_H
#define EMERGENCY_H

#include "Config.h"
#include "MAX30102_Module.h"
#include "PulseSensor_Module.h"
#include "DS18B20_Module.h"
#include "ECG_Module.h"
#include "GPS_Module.h"
#include "GSM_Module.h"

// ---------------------------------------------------------------------------
// Emergency Module
// Evaluates vital signs against thresholds and triggers SMS alerts
// with patient location when an emergency condition is detected.
// ---------------------------------------------------------------------------

enum EmergencyLevel {
  EMERGENCY_NONE = 0,
  EMERGENCY_WARNING,
  EMERGENCY_CRITICAL
};

struct EmergencyStatus {
  EmergencyLevel level;
  bool           active;
  bool           smsSent;
  char           reason[64];
};

class EmergencyHandler {
public:
  bool init();
  void update();
  void clear();

  const EmergencyStatus &getStatus() const;
  void triggerManualEmergency();
  void sendHealthReport();

private:
  EmergencyStatus _status;
  unsigned long   _lastAlertMs;
  unsigned long   _lastHealthReportMs;

  EmergencyLevel evaluateVitals();
  void dispatchAlert(EmergencyLevel level, const char *reason);
  void buildVitalsMessage(char *buffer, size_t bufferSize, const char *prefix, const char *reason);
};

extern EmergencyHandler emergencyHandler;

#endif // EMERGENCY_H
