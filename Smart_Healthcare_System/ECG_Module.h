#ifndef ECG_MODULE_H
#define ECG_MODULE_H

#include "Config.h"

// ---------------------------------------------------------------------------
// ECG Module
// Reads analog output from an AD8232 ECG front-end.
// Provides filtered sample value and basic rhythm indicator.
// ---------------------------------------------------------------------------

struct ECGData {
  int   rawValue;
  int   filteredValue;
  int   baseline;
  bool  leadOff;
  bool  sensorReady;
};

class ECGModule {
public:
  bool init();
  void update();
  void clear();

  const ECGData &getData() const;

private:
  ECGData       _data;
  unsigned long _lastSampleMs;
  long          _baselineSum;
  int           _baselineCount;

  int readAnalogSample();
  int applyLowPassFilter(int sample);
};

extern ECGModule ecgModule;

#endif // ECG_MODULE_H
