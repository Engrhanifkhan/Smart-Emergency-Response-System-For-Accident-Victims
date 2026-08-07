#include "ECG_Module.h"

ECGModule ecgModule;

bool ECGModule::init() {
  clear();
  pinMode(ECG_ANALOG_PIN, INPUT);

  // Pull-downs: if LO+/LO- wires are open, treat as leads ON (not floating HIGH)
  pinMode(ECG_LO_PLUS_PIN, INPUT_PULLDOWN);
  pinMode(ECG_LO_MINUS_PIN, INPUT_PULLDOWN);

#ifdef ESP32
  analogSetPinAttenuation(ECG_ANALOG_PIN, ADC_11db);
#endif

  delay(50);

  // Seed filter with current ADC
  long sum = 0;
  for (int i = 0; i < 10; ++i) {
    sum += analogRead(ECG_ANALOG_PIN);
    delay(5);
  }
  _filterState = (float)(sum / 10);
  _data.baseline = (int)_filterState;
  _data.sensorReady = true;

  DEBUG_PRINT(F("[ECG] AD8232 GPIO "));
  DEBUG_PRINT(ECG_ANALOG_PIN);
  DEBUG_PRINT(F(" LO+="));
  DEBUG_PRINT(ECG_LO_PLUS_PIN);
  DEBUG_PRINT(F(" LO-="));
  DEBUG_PRINTLN(ECG_LO_MINUS_PIN);
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

  const bool loPlus  = digitalRead(ECG_LO_PLUS_PIN) == HIGH;
  const bool loMinus = digitalRead(ECG_LO_MINUS_PIN) == HIGH;
  _data.leadOff = (loPlus || loMinus);

  // Always filter/plot raw signal so display works even if LO pins miswired
  _data.filteredValue = applyLowPassFilter(_data.rawValue);

  if (_baselineCount < ECG_BASELINE_SAMPLES) {
    _baselineSum += _data.filteredValue;
    ++_baselineCount;
    _data.baseline = (int)(_baselineSum / _baselineCount);
  } else {
    // Slow baseline track for graph centering
    _data.baseline = (int)(0.995f * _data.baseline + 0.005f * _data.filteredValue);
  }
}

void ECGModule::clear() {
  _data.rawValue      = 0;
  _data.filteredValue = 0;
  _data.baseline      = ADC_MID_SCALE;
  _data.leadOff       = false;
  _data.sensorReady   = false;
  _lastSampleMs       = 0;
  _baselineSum        = 0;
  _baselineCount      = 0;
  _filterState        = (float)ADC_MID_SCALE;
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
  const float alpha = ECG_FILTER_ALPHA;
  _filterState = alpha * sample + (1.0f - alpha) * _filterState;
  return (int)_filterState;
}
