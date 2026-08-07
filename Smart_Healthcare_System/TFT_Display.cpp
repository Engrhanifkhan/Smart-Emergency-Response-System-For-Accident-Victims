#include "TFT_Display.h"
#include <SPI.h>
#include <string.h>

TFTDisplay tftDisplay;

bool TFTDisplay::init() {
  // Proven HX8357 init (same as working LCD test)
  _tft.begin(HX8357D);
  _tft.setRotation(1);

#if TFT_MIRROR_FIX
  _tft.startWrite();
  _tft.writeCommand(0x36);
  SPI.transfer(TFT_MADCTL_VALUE);
  _tft.endWrite();
#endif

  _lastVitalsMs = 0;
  _layoutReady  = false;
  _ecgX         = ECG_GRAPH_X;
  _lastEcgY     = ECG_GRAPH_Y + (ECG_GRAPH_H / 2);
  memset(&_lastData, 0, sizeof(_lastData));

  // Project name splash — minimum 10 seconds
  showBootScreen();
  delay(10000);
  drawStaticLayout();

  DEBUG_PRINTLN(F("[TFT] Display ON — sensors optional."));
  return true;
}

void TFTDisplay::showHardwareTestScreen() {
  // kept for optional manual call; not used at boot
  showBootScreen();
}

void TFTDisplay::showBootScreen() {
  _tft.fillScreen(HX8357_NAVY);
  _tft.drawRect(2, 2, TFT_W - 4, TFT_H - 4, HX8357_CYAN);

  _tft.setTextColor(HX8357_CYAN);
  _tft.setTextSize(2);
  _tft.setCursor(20, 50);
  _tft.println(F(PROJECT_NAME_LINE1));

  _tft.setTextColor(HX8357_WHITE);
  _tft.setTextSize(2);
  _tft.setCursor(90, 95);
  _tft.println(F(PROJECT_NAME_LINE2));

  _tft.setCursor(70, 130);
  _tft.println(F(PROJECT_NAME_LINE3));

  _tft.setCursor(70, 165);
  _tft.println(F(PROJECT_NAME_LINE4));

  _tft.setTextColor(HX8357_YELLOW);
  _tft.setTextSize(1);
  _tft.setCursor(150, 240);
  _tft.println(F("Starting... 10s"));
}

void TFTDisplay::showMessage(const char *title, const char *message, uint16_t color) {
  _tft.fillRect(0, 50, TFT_W, 60, HX8357_BLACK);
  _tft.setTextColor(color);
  _tft.setTextSize(3);
  _tft.setCursor(12, 55);
  _tft.println(title);
  _tft.setTextSize(2);
  _tft.setCursor(12, 90);
  _tft.println(message);
}

void TFTDisplay::clear() {
  _tft.fillScreen(HX8357_BLACK);
  _layoutReady = false;
}

void TFTDisplay::drawStaticLayout() {
  _tft.fillScreen(HX8357_BLACK);

  _tft.fillRect(0, 0, TFT_W, 28, HX8357_DARKGREEN);
  _tft.setTextColor(HX8357_WHITE);
  _tft.setTextSize(1);
  _tft.setCursor(6, 4);
  _tft.print(F("Smart Emergency Response System"));
  _tft.setCursor(6, 16);
  _tft.print(F("Accident Victims Vital Sign Monitor"));

  // Labels + human body ranges
  _tft.setTextSize(2);
  _tft.setTextColor(HX8357_CYAN);
  _tft.setCursor(16, 34);
  _tft.print(F("TEMP"));
  _tft.setTextSize(1);
  _tft.setTextColor(HX8357_WHITE);
  _tft.setCursor(16, 52);
  _tft.print((int)TEMP_MIN_C);
  _tft.print('-');
  _tft.print(TEMP_MAX_C, 1);
  _tft.print(F("C / F"));

  _tft.setTextSize(2);
  _tft.setTextColor(HX8357_YELLOW);
  _tft.setCursor(175, 34);
  _tft.print(F("HR bpm"));
  _tft.setTextSize(1);
  _tft.setTextColor(HX8357_WHITE);
  _tft.setCursor(175, 52);
  _tft.print(PULSE_MIN_BPM);
  _tft.print('-');
  _tft.print(PULSE_MAX_BPM);
  _tft.print(F(" bpm"));

  _tft.setTextSize(2);
  _tft.setTextColor(HX8357_MAGENTA);
  _tft.setCursor(340, 34);
  _tft.print(F("SpO2 %"));
  _tft.setTextSize(1);
  _tft.setTextColor(HX8357_WHITE);
  _tft.setCursor(340, 52);
  _tft.print(SPO2_MIN_PCT);
  _tft.print('-');
  _tft.print(SPO2_MAX_PCT);
  _tft.print(F(" %"));

  _tft.setTextSize(1);
  _tft.setTextColor(HX8357_ORANGE);
  _tft.setCursor(16, 108);
  _tft.print(F("Pulse Amped "));
  _tft.print(PULSE_MIN_BPM);
  _tft.print('-');
  _tft.print(PULSE_MAX_BPM);
  _tft.print(F(" bpm:"));

  _tft.drawRect(ECG_GRAPH_X - 1, ECG_GRAPH_Y - 1, ECG_GRAPH_W + 2, ECG_GRAPH_H + 2, HX8357_DARKGREY);
  _tft.setTextColor(HX8357_GREEN);
  _tft.setCursor(ECG_GRAPH_X, ECG_GRAPH_Y - 12);
  _tft.print(F("ECG AD8232  "));
  _tft.print(ECG_MIN_BPM);
  _tft.print('-');
  _tft.print(ECG_MAX_BPM);
  _tft.print(F(" bpm  (+/-"));
  _tft.print(ECG_ADC_SPAN);
  _tft.print(F(" ADC)"));

  const int midY = ECG_GRAPH_Y + (ECG_GRAPH_H / 2);
  for (int x = ECG_GRAPH_X; x < ECG_GRAPH_X + ECG_GRAPH_W; x += 6) {
    _tft.drawPixel(x, midY, HX8357_DARKGREY);
  }

  _layoutReady = true;
  _ecgX = ECG_GRAPH_X;
  _lastEcgY = midY;
}

void TFTDisplay::update(const DisplayData &data) {
  if (!_layoutReady) {
    drawStaticLayout();
  }

  const unsigned long now = millis();

  if (vitalsChanged(data) || (now - _lastVitalsMs) >= DISPLAY_REFRESH_INTERVAL_MS) {
    drawVitals(data);
    _lastVitalsMs = now;
  }

  if (statusChanged(data)) {
    drawStatusBar(data);
  }

  plotEcgSample(data.ecgValue, data.ecgBaseline, data.ecgLeadOff, data.ecgOk);

  _lastData = data;
}

void TFTDisplay::drawVitals(const DisplayData &data) {
  // Clear value area (keep range labels at y=52)
  _tft.fillRect(0, 62, TFT_W, 44, HX8357_BLACK);
  _tft.fillRect(150, 106, 220, 16, HX8357_BLACK);

  _tft.setTextSize(3);

  // Temperature — Celsius + Fahrenheit (red if outside body range)
  const bool tempValid = data.tempOk && data.bodyTemperatureC > 1.0f;
  const bool tempWarn  = tempValid &&
      (data.bodyTemperatureC < TEMP_MIN_C || data.bodyTemperatureC > TEMP_MAX_C);
  _tft.setTextColor(!tempValid ? HX8357_DARKGREY : (tempWarn ? HX8357_RED : HX8357_CYAN));
  _tft.setCursor(12, 64);
  if (tempValid) {
    _tft.setTextSize(2);
    _tft.print(data.bodyTemperatureC, 1);
    _tft.print(F("C "));
    _tft.print(C_TO_F(data.bodyTemperatureC), 1);
    _tft.print(F("F"));
  } else {
    _tft.setTextSize(3);
    _tft.print(F("--"));
  }

  // Heart rate
  const int hr = (data.heartRateBpm > 0) ? data.heartRateBpm : data.pulseSensorBpm;
  const bool hrValid = hr > 0;
  const bool hrWarn  = hrValid && (hr < HR_LOW_THRESHOLD_BPM || hr > HR_HIGH_THRESHOLD_BPM);
  _tft.setTextColor(!hrValid ? HX8357_DARKGREY : (hrWarn ? HX8357_RED : HX8357_YELLOW));
  _tft.setTextSize(3);
  _tft.setCursor(175, 64);
  if (hrValid) {
    _tft.print(hr);
  } else {
    _tft.print(F("--"));
  }

  // SpO2 — red if below critical / outside normal band
  const bool spo2Valid = data.max30102Ok && data.spo2Percent > 0;
  const bool spo2Warn  = spo2Valid &&
      (data.spo2Percent < SPO2_MIN_PCT || data.spo2Percent > SPO2_MAX_PCT);
  _tft.setTextColor(!spo2Valid ? HX8357_DARKGREY : (spo2Warn ? HX8357_RED : HX8357_MAGENTA));
  _tft.setCursor(340, 64);
  if (spo2Valid) {
    _tft.print(data.spo2Percent);
  } else {
    _tft.print(F("--"));
  }

  // PulseSensor Amped value
  _tft.setTextSize(2);
  const bool pulseWarn = data.pulseOk && data.pulseSensorBpm > 0 &&
      (data.pulseSensorBpm < PULSE_MIN_BPM || data.pulseSensorBpm > PULSE_MAX_BPM);
  _tft.setTextColor(!data.pulseOk ? HX8357_DARKGREY :
                    (pulseWarn ? HX8357_RED : HX8357_ORANGE));
  _tft.setCursor(155, 106);
  if (!data.pulseOk) {
    _tft.print(F("OFF"));
  } else if (data.pulseSensorBpm > 0) {
    _tft.print(data.pulseSensorBpm);
    _tft.print(F(" bpm"));
    if (pulseWarn) {
      _tft.print(F(" !"));
    }
  } else {
    _tft.print(F("raw "));
    _tft.print(data.pulseRaw);
  }
}

void TFTDisplay::drawStatusBar(const DisplayData &data) {
  _tft.fillRect(0, 272, TFT_W, 48, HX8357_DARKGREY);
  _tft.setTextSize(1);

  // Module connection row
  _tft.setCursor(6, 276);
  _tft.setTextColor(data.tempOk ? HX8357_GREEN : HX8357_RED);
  _tft.print(F("TEMP:"));
  _tft.print(data.tempOk ? F("OK ") : F("OFF "));

  _tft.setTextColor(data.max30102Ok ? HX8357_GREEN : HX8357_RED);
  _tft.print(F("MAX:"));
  _tft.print(data.max30102Ok ? F("OK ") : F("OFF "));

  _tft.setTextColor(data.pulseOk ? HX8357_GREEN : HX8357_RED);
  _tft.print(F("PUL:"));
  _tft.print(data.pulseOk ? F("OK ") : F("OFF "));

  _tft.setTextColor(data.ecgOk ? HX8357_GREEN : HX8357_RED);
  _tft.print(F("ECG:"));
  _tft.print(data.ecgOk ? F("OK ") : F("OFF "));

  _tft.setTextColor(data.gpsOk ? HX8357_GREEN : HX8357_RED);
  _tft.print(F("GPS:"));
  _tft.print(data.gpsOk ? F("OK ") : F("OFF "));

  _tft.setTextColor(data.gsmOk ? HX8357_GREEN : HX8357_RED);
  _tft.print(F("GSM:"));
  _tft.print(data.gsmOk ? F("OK") : F("OFF"));

  // GPS / GSM detail
  _tft.setTextSize(1);
  _tft.setTextColor(HX8357_WHITE);
  _tft.setCursor(6, 292);
  _tft.print(F("GPS "));
  if (data.gpsOk && data.gpsFix) {
    _tft.print(F("FIX "));
    _tft.print(data.latitude, 4);
    _tft.print(',');
    _tft.print(data.longitude, 4);
  } else if (data.gpsOk) {
    _tft.print(F("searching..."));
  } else {
    _tft.print(F("not connected"));
  }

  _tft.setCursor(6, 306);
  _tft.print(F("GSM "));
  if (data.gsmReady) {
    _tft.print(F("READY  SMS ok"));
  } else if (data.gsmOk) {
    _tft.print(F("modem OK, net wait"));
  } else {
    _tft.print(F("not connected"));
  }

  if (data.emergencyActive) {
    _tft.setTextColor(HX8357_RED);
    _tft.setTextSize(2);
    _tft.setCursor(400, 292);
    _tft.print(F("SOS"));
  }
}

int16_t TFTDisplay::mapEcgToY(int ecgValue, int baseline) const {
  const int mid = (baseline > 0) ? baseline : ADC_MID_SCALE;
  const int span = ECG_ADC_SPAN;
  int delta = ecgValue - mid;
  if (delta < -span) delta = -span;
  if (delta > span)  delta = span;

  const int y = ECG_GRAPH_Y + (ECG_GRAPH_H / 2) - ((delta * (ECG_GRAPH_H / 2)) / span);
  if (y < ECG_GRAPH_Y) return ECG_GRAPH_Y;
  if (y > ECG_GRAPH_Y + ECG_GRAPH_H - 1) return ECG_GRAPH_Y + ECG_GRAPH_H - 1;
  return (int16_t)y;
}

void TFTDisplay::plotEcgSample(int ecgValue, int baseline, bool leadOff, bool ecgOk) {
  if (!ecgOk) {
    const int midY = ECG_GRAPH_Y + (ECG_GRAPH_H / 2);
    _tft.drawFastVLine(_ecgX, ECG_GRAPH_Y, ECG_GRAPH_H, HX8357_BLACK);
    _tft.drawPixel(_ecgX, midY, HX8357_DARKGREY);
    if (_ecgX == ECG_GRAPH_X) {
      _tft.setTextSize(1);
      _tft.setTextColor(HX8357_DARKGREY, HX8357_BLACK);
      _tft.setCursor(ECG_GRAPH_X + 140, ECG_GRAPH_Y + 4);
      _tft.print(F("ECG NOT CONNECTED"));
    }
    _lastEcgY = midY;
  } else {
    // Always plot waveform (even if LO pins say lead-off)
    const int16_t y = mapEcgToY(ecgValue, baseline);
    _tft.drawFastVLine(_ecgX, ECG_GRAPH_Y, ECG_GRAPH_H, HX8357_BLACK);
    const uint16_t color = leadOff ? HX8357_ORANGE : HX8357_GREEN;
    if (_ecgX > ECG_GRAPH_X) {
      _tft.drawLine(_ecgX - 1, _lastEcgY, _ecgX, y, color);
    } else {
      _tft.drawPixel(_ecgX, y, color);
    }
    _lastEcgY = y;

    if (leadOff && _ecgX == ECG_GRAPH_X) {
      _tft.setTextSize(1);
      _tft.setTextColor(HX8357_ORANGE, HX8357_BLACK);
      _tft.setCursor(ECG_GRAPH_X + 160, ECG_GRAPH_Y + 4);
      _tft.print(F("LEAD OFF"));
    }
  }

  const int eraseX = _ecgX + 3;
  if (eraseX < ECG_GRAPH_X + ECG_GRAPH_W) {
    _tft.drawFastVLine(eraseX, ECG_GRAPH_Y, ECG_GRAPH_H, HX8357_BLACK);
  }

  _ecgX++;
  if (_ecgX >= ECG_GRAPH_X + ECG_GRAPH_W) {
    _ecgX = ECG_GRAPH_X;
    const int midY = ECG_GRAPH_Y + (ECG_GRAPH_H / 2);
    for (int x = ECG_GRAPH_X; x < ECG_GRAPH_X + ECG_GRAPH_W; x += 8) {
      _tft.drawPixel(x, midY, HX8357_DARKGREY);
    }
  }
}

bool TFTDisplay::vitalsChanged(const DisplayData &data) const {
  return (data.bodyTemperatureC != _lastData.bodyTemperatureC) ||
         (data.heartRateBpm      != _lastData.heartRateBpm) ||
         (data.pulseSensorBpm    != _lastData.pulseSensorBpm) ||
         (data.pulseRaw          != _lastData.pulseRaw) ||
         (data.spo2Percent       != _lastData.spo2Percent) ||
         (data.tempOk            != _lastData.tempOk) ||
         (data.max30102Ok        != _lastData.max30102Ok) ||
         (data.pulseOk           != _lastData.pulseOk);
}

bool TFTDisplay::statusChanged(const DisplayData &data) const {
  return (data.gpsFix != _lastData.gpsFix) ||
         (data.gsmReady != _lastData.gsmReady) ||
         (data.emergencyActive != _lastData.emergencyActive) ||
         (data.latitude != _lastData.latitude) ||
         (data.longitude != _lastData.longitude) ||
         (data.tempOk != _lastData.tempOk) ||
         (data.max30102Ok != _lastData.max30102Ok) ||
         (data.pulseOk != _lastData.pulseOk) ||
         (data.ecgOk != _lastData.ecgOk) ||
         (data.gpsOk != _lastData.gpsOk) ||
         (data.gsmOk != _lastData.gsmOk);
}
