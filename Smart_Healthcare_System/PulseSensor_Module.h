#ifndef PULSE_SENSOR_MODULE_H
#define PULSE_SENSOR_MODULE_H

#include "Config.h"

// ---------------------------------------------------------------------------
// Pulse Sensor Module
// Reads analog PPG signal from a pulse sensor on A1.
// Computes BPM via peak detection on the filtered waveform.
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
  int             _peakThreshold;
  bool            _beatInProgress;

  int  readSample();
  int  filterSample(int sample);
  void detectBeat(int sample);
};

extern PulseSensorModule pulseSensor;

#endif // PULSE_SENSOR_MODULE_H
