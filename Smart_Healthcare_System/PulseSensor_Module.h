#ifndef PULSE_SENSOR_MODULE_H
#define PULSE_SENSOR_MODULE_H

#include "Config.h"

// ---------------------------------------------------------------------------
// PulseSensor Amped — analog PPG on ESP32 ADC (GPIO 34)
// Adaptive baseline peak detection (works with varying ADC levels)
// ---------------------------------------------------------------------------

struct PulseSensorData {
  int  rawValue;
  int  bpm;
  bool fingerDetected;
  bool sensorReady;
};

class PulseSensorModule {
public:
  bool init();
  void update();
  void clear();

  const PulseSensorData &getData() const;

private:
  PulseSensorData _data;
  unsigned long   _lastSampleMs;
  unsigned long   _lastBeatMs;
  float           _baseline;
  int             _peakThreshold;
  bool            _beatInProgress;
  int             _sampleMin;
  int             _sampleMax;
  unsigned long   _windowStartMs;

  int  readSample();
  void detectBeat(int sample);
  void updateFingerWindow(int sample);
};

extern PulseSensorModule pulseSensor;

#endif // PULSE_SENSOR_MODULE_H
