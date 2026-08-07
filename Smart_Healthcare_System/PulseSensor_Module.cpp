#include "PulseSensor_Module.h"

PulseSensorModule pulseSensor;

bool PulseSensorModule::init() {
  clear();
  pinMode(PULSE_SENSOR_PIN, INPUT);

#ifdef ESP32
  // Full 0–3.3 V range for PulseSensor Amped
  analogSetPinAttenuation(PULSE_SENSOR_PIN, ADC_11db);
#endif

  // Seed baseline from a few reads
  long sum = 0;
  for (int i = 0; i < 20; ++i) {
    sum += analogRead(PULSE_SENSOR_PIN);
    delay(5);
  }
  _baseline = (float)(sum / 20);
  _peakThreshold = (int)_baseline + 80;
  _data.sensorReady = true;

  DEBUG_PRINT(F("[PulseSensor] Amped GPIO "));
  DEBUG_PRINT(PULSE_SENSOR_PIN);
  DEBUG_PRINT(F(" baseline="));
  DEBUG_PRINTLN((int)_baseline);
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
  updateFingerWindow(sample);

  // Slow baseline track (PulseSensor Amped sits near mid-rail)
  _baseline = 0.95f * _baseline + 0.05f * (float)sample;
  _peakThreshold = (int)_baseline + 60;

  if (!_data.fingerDetected) {
    _data.bpm = 0;
    _beatInProgress = false;
    return;
  }

  detectBeat(sample);
}

void PulseSensorModule::clear() {
  _data.rawValue       = 0;
  _data.bpm            = 0;
  _data.fingerDetected = false;
  _data.sensorReady    = false;
  _lastSampleMs        = 0;
  _lastBeatMs          = 0;
  _baseline            = (float)ADC_MID_SCALE;
  _peakThreshold       = ADC_MID_SCALE + 80;
  _beatInProgress      = false;
  _sampleMin           = 4095;
  _sampleMax           = 0;
  _windowStartMs       = 0;
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

void PulseSensorModule::updateFingerWindow(int sample) {
  const unsigned long now = millis();
  if (_windowStartMs == 0) {
    _windowStartMs = now;
  }

  if (sample < _sampleMin) _sampleMin = sample;
  if (sample > _sampleMax) _sampleMax = sample;

  // Every 1 s: finger present if signal has some pulse amplitude
  if ((now - _windowStartMs) >= 1000UL) {
    const int span = _sampleMax - _sampleMin;
    // PulseSensor Amped usually shows 30–400 ADC counts of swing with finger
    _data.fingerDetected = (span >= 25 && sample > 100);
    _sampleMin = sample;
    _sampleMax = sample;
    _windowStartMs = now;
  }
}

void PulseSensorModule::detectBeat(int sample) {
  static int lastSample = ADC_MID_SCALE;

  if (!_beatInProgress && sample > _peakThreshold && lastSample <= _peakThreshold) {
    const unsigned long now = millis();
    if (_lastBeatMs > 0) {
      const unsigned long interval = now - _lastBeatMs;
      if (interval > 300 && interval < 2000) {
        const int instant = (int)(60000UL / interval);
        // Light smoothing
        if (_data.bpm == 0) {
          _data.bpm = instant;
        } else {
          _data.bpm = (int)(0.7f * _data.bpm + 0.3f * instant);
        }
      }
    }
    _lastBeatMs = now;
    _beatInProgress = true;
  }

  if (sample < (int)_baseline - 20) {
    _beatInProgress = false;
  }

  lastSample = sample;
}
