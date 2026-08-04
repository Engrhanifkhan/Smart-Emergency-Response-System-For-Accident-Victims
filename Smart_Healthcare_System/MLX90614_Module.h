#ifndef MLX90614_MODULE_H
#define MLX90614_MODULE_H

#include "Config.h"
#include <Wire.h>

// ---------------------------------------------------------------------------
// MLX90614 Module
// Uses I2C Bus 2 (Wire1 on ESP32) — separate SDA/SCL from MAX30102.
// ---------------------------------------------------------------------------

struct MLX90614Data {
  float ambientTemperatureC;
  float objectTemperatureC;
  bool  sensorReady;
};

class MLX90614Module {
public:
  bool init();
  void update();
  void clear();

  const MLX90614Data &getData() const;

private:
  MLX90614Data  _data;
  unsigned long _lastSampleMs;

  float readTemperature(uint8_t reg);
};

extern MLX90614Module mlx90614;

#endif // MLX90614_MODULE_H
