#include "DS18B20_Module.h"

DS18B20Module ds18b20;

bool DS18B20Module::init() {
  clear();

  pinMode(DS18B20_PIN, INPUT_PULLUP);
  _sensors.begin();
  _sensors.setWaitForConversion(false);

  const int count = _sensors.getDeviceCount();
  DEBUG_PRINT(F("[DS18B20] OneWire pin GPIO "));
  DEBUG_PRINT(DS18B20_PIN);
  DEBUG_PRINT(F(" devices="));
  DEBUG_PRINTLN(count);

  if (count < 1) {
    DEBUG_PRINTLN(F("[DS18B20] Sensor not detected (need 4.7k pull-up to 3.3V)."));
    return false;
  }

  _sensors.setResolution(12);
  _sensors.requestTemperatures();
  _lastSampleMs = millis();

  _data.sensorReady = true;
  DEBUG_PRINTLN(F("[DS18B20] Body temperature sensor ready."));
  return true;
}

void DS18B20Module::update() {
  if (!_data.sensorReady) {
    return;
  }

  const unsigned long now = millis();
  if ((now - _lastSampleMs) < SENSOR_SAMPLE_INTERVAL_MS) {
    return;
  }
  _lastSampleMs = now;

  // Non-blocking style: read last conversion, then request next
  const float t = _sensors.getTempCByIndex(0);
  if (t > -55.0f && t < 125.0f && t != DEVICE_DISCONNECTED_C) {
    _data.temperatureC = t;
  } else {
    DEBUG_PRINTLN(F("[DS18B20] Read failed."));
  }

  _sensors.requestTemperatures();
}

void DS18B20Module::clear() {
  _data.temperatureC = 0.0f;
  _data.sensorReady  = false;
  _lastSampleMs      = 0;
}

const DS18B20Data &DS18B20Module::getData() const {
  return _data;
}
