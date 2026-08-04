#ifndef MAX30102_MODULE_H
#define MAX30102_MODULE_H

#include "Config.h"
#include <Wire.h>

// ---------------------------------------------------------------------------
// MAX30102 Module
// Pulse oximeter: heart rate (BPM) and SpO2 (%).
// Uses I2C Bus 1 (Wire) — separate SDA/SCL from MLX90614.
// ---------------------------------------------------------------------------

struct MAX30102Data {
  int  heartRateBpm;
  int  spo2Percent;
  bool fingerDetected;
  bool sensorReady;
};

class MAX30102Module {
public:
  bool init();
  void update();
  void clear();

  const MAX30102Data &getData() const;

private:
  MAX30102Data    _data;
  unsigned long   _lastSampleMs;

  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegisters(uint8_t reg, uint8_t *buffer, uint8_t length);
  void readFifoSamples();
  void estimateVitals();
};

extern MAX30102Module max30102;

#endif // MAX30102_MODULE_H
