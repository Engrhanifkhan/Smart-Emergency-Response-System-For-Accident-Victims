#ifndef DS18B20_MODULE_H
#define DS18B20_MODULE_H

#include "Config.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------------------------------------------------------------------------
// DS18B20 Module — waterproof / probe body temperature (OneWire)
// ---------------------------------------------------------------------------

struct DS18B20Data {
  float temperatureC;
  bool  sensorReady;
};

class DS18B20Module {
public:
  bool init();
  void update();
  void clear();

  const DS18B20Data &getData() const;

private:
  OneWire           _oneWire{DS18B20_PIN};
  DallasTemperature _sensors{&_oneWire};
  DS18B20Data       _data;
  unsigned long     _lastSampleMs;
};

extern DS18B20Module ds18b20;

#endif // DS18B20_MODULE_H
