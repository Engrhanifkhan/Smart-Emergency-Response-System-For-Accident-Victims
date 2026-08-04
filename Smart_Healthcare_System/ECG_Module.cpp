#include "ECG_Module.h"

ECGModule ecgModule;

bool ECGModule::init() {
  clear();
  pinMode(ECG_ANALOG_PIN, INPUT);
  pinMode(ECG_LO_PLUS_PIN, INPUT);
  pinMode(ECG_LO_MINUS_PIN, INPUT);

  delay(100);
  _data.sensorReady = true;
  DEBUG_PRINT(F("[ECG] Ready on GPIO "));
  DEBUG_PRINTLN(ECG_ANALOG_PIN);
  return true;
}

void ECGModule::update() {
  if (!_data.sensorReady) {
    return;
  }

  const unsigned long intervalMs = 1000UL / ECG_SAMPLE_RATE_HZ;
  const unsigned long now = millis();
  if ((now - _lastSampleMs) < intervalMs) {
    return;
  }
  _lastSampleMs = now;

  _data.rawValue = readAnalogSample();

  _data.leadOff = (digitalRead(ECG_LO_PLUS_PIN) == HIGH ||
                   digitalRead(ECG_LO_MINUS_PIN) == HIGH);

  if (_data.leadOff) {
    _data.filteredValue = 0;
    return;
  }

  _data.filteredValue = applyLowPassFilter(_data.rawValue);

  if (_baselineCount < ECG_BASELINE_SAMPLES) {
    _baselineSum += _data.filteredValue;
    ++_baselineCount;
    _data.baseline = (int)(_baselineSum / _baselineCount);
  }
}

void ECGModule::clear() {
  _data.rawValue      = 0;
  _data.filteredValue = 0;
  _data.baseline      = ADC_MID_SCALE;
  _data.leadOff       = true;
  _data.sensorReady   = false;
  _lastSampleMs       = 0;
  _baselineSum        = 0;
  _baselineCount      = 0;
}

const ECGData &ECGModule::getData() const {
  return _data;
}

int ECGModule::readAnalogSample() {
  long sum = 0;
  for (int i = 0; i < 4; ++i) {
    sum += analogRead(ECG_ANALOG_PIN);
    delayMicroseconds(200);
  }
  return (int)(sum / 4);
}

int ECGModule::applyLowPassFilter(int sample) {
  static float filtered = (float)ADC_MID_SCALE;
  const float alpha = 0.15f;
  filtered = alpha * sample + (1.0f - alpha) * filtered;
  return (int)filtered;
}
