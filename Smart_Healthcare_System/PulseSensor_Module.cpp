#include "PulseSensor_Module.h"

PulseSensorModule pulseSensor;

bool PulseSensorModule::init() {
  clear();
  pinMode(PULSE_SENSOR_PIN, INPUT);
  _peakThreshold = PULSE_PEAK_THRESHOLD;
  _data.sensorReady = true;
  DEBUG_PRINT(F("[PulseSensor] Ready on GPIO "));
  DEBUG_PRINTLN(PULSE_SENSOR_PIN);
  return true;
}

void PulseSensorModule::update() {
  if (!_data.sensorReady) {
    return;
  }

  const unsigned long intervalMs = 1000UL / PULSE_SAMPLE_RATE_HZ;
  const unsigned long now = millis();
  if ((now - _lastSampleMs) < intervalMs) {
    return;
  }
  _lastSampleMs = now;

  const int sample = readSample();
  _data.rawValue = sample;

  _data.fingerDetected = (sample > PULSE_FINGER_MIN && sample < PULSE_FINGER_MAX);
  if (!_data.fingerDetected) {
    _data.bpm = 0;
    _beatInProgress = false;
    return;
  }

  const int filtered = filterSample(sample);
  detectBeat(filtered);
}

void PulseSensorModule::clear() {
  _data.rawValue       = 0;
  _data.bpm            = 0;
  _data.fingerDetected = false;
  _data.sensorReady    = false;
  _lastSampleMs        = 0;
  _lastBeatMs          = 0;
  _beatInProgress      = false;
}

const PulseSensorData &PulseSensorModule::getData() const {
  return _data;
}

int PulseSensorModule::readSample() {
  long sum = 0;
  for (int i = 0; i < 4; ++i) {
    sum += analogRead(PULSE_SENSOR_PIN);
    delayMicroseconds(200);
  }
  return (int)(sum / 4);
}

int PulseSensorModule::filterSample(int sample) {
  static float filtered = (float)ADC_MID_SCALE;
  const float alpha = 0.2f;
  filtered = alpha * sample + (1.0f - alpha) * filtered;
  return (int)filtered;
}

void PulseSensorModule::detectBeat(int sample) {
  static int lastSample = ADC_MID_SCALE;

  if (!_beatInProgress && sample > _peakThreshold && lastSample <= _peakThreshold) {
    const unsigned long now = millis();
    if (_lastBeatMs > 0) {
      const unsigned long interval = now - _lastBeatMs;
      if (interval > 300 && interval < 2000) {
        _data.bpm = (int)(60000UL / interval);
      }
    }
    _lastBeatMs = now;
    _beatInProgress = true;
  }

  if (sample < _peakThreshold - 80) {
    _beatInProgress = false;
  }

  lastSample = sample;
}
