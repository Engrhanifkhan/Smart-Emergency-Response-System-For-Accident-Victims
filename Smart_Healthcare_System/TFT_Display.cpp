#include "TFT_Display.h"
#include <SPI.h>
#include <string.h>

TFTDisplay tftDisplay;

static void applyDisplayOrientation(Adafruit_ILI9341 &tft) {
  tft.setRotation(1);

#if TFT_MIRROR_FIX
  tft.writeCommand(0x36);
  SPI.transfer(TFT_MADCTL_VALUE);
#endif
}

static void resetDisplayHardware() {
  pinMode(TFT_RST_PIN, OUTPUT);
  pinMode(TFT_CS_PIN, OUTPUT);
  pinMode(TFT_DC_PIN, OUTPUT);

#if !defined(ESP32)
  pinMode(TFT_SPI_SS_PIN, OUTPUT);
  digitalWrite(TFT_SPI_SS_PIN, HIGH);
#endif

  digitalWrite(TFT_CS_PIN, HIGH);
  digitalWrite(TFT_DC_PIN, HIGH);

  digitalWrite(TFT_RST_PIN, HIGH);
  delay(10);
  digitalWrite(TFT_RST_PIN, LOW);
  delay(20);
  digitalWrite(TFT_RST_PIN, HIGH);
  delay(150);
}

bool TFTDisplay::init() {
  resetDisplayHardware();
  SPI.begin();

#if !defined(ESP32)
  SPI.setClockDivider(SPI_CLOCK_DIV4);
#endif

  _tft.begin();

#if !defined(ESP32)
  _tft.setSPISpeed(4000000);
#endif

  applyDisplayOrientation(_tft);

  _lastVitalsMs = 0;
  _layoutReady  = false;
  _ecgX         = ECG_GRAPH_X;
  _lastEcgY     = ECG_GRAPH_Y + (ECG_GRAPH_H / 2);
  memset(&_lastData, 0, sizeof(_lastData));

  showHardwareTestScreen();
  delay(600);
  showBootScreen();
  delay(400);

  drawStaticLayout();
  DEBUG_PRINTLN(F("[TFT] Dashboard + live ECG graph ready."));
  return true;
}

void TFTDisplay::showHardwareTestScreen() {
  _tft.fillScreen(ILI9341_WHITE);
  _tft.drawRect(5, 5, 310, 230, ILI9341_RED);
  _tft.setTextColor(ILI9341_BLACK);
  _tft.setTextSize(2);
  _tft.setCursor(20, 50);
  _tft.println(F("ENGR HANIF KHAN"));
  _tft.setTextSize(1);
  _tft.setCursor(20, 80);
#ifdef ESP32
  _tft.println(F("ESP32-WROOM LCD OK"));
#else
  _tft.println(F("MEGA LCD TEST OK"));
#endif
}

void TFTDisplay::showBootScreen() {
  _tft.fillScreen(ILI9341_NAVY);
  _tft.setTextColor(ILI9341_WHITE);
  _tft.setTextSize(2);
  _tft.setCursor(20, 100);
  _tft.println(F("Smart Healthcare"));
  _tft.setTextSize(1);
  _tft.setCursor(50, 130);
  _tft.println(F("Loading live ECG dashboard..."));
}

void TFTDisplay::showMessage(const char *title, const char *message, uint16_t color) {
  _tft.fillRect(0, 40, 320, 50, ILI9341_BLACK);
  _tft.setTextColor(color);
  _tft.setTextSize(2);
  _tft.setCursor(8, 45);
  _tft.println(title);
  _tft.setTextSize(1);
  _tft.setCursor(8, 70);
  _tft.println(message);
}

void TFTDisplay::clear() {
  _tft.fillScreen(ILI9341_BLACK);
  _layoutReady = false;
}

void TFTDisplay::drawStaticLayout() {
  _tft.fillScreen(ILI9341_BLACK);

  // Header
  _tft.fillRect(0, 0, 320, 22, ILI9341_DARKGREEN);
  _tft.setTextColor(ILI9341_WHITE);
  _tft.setTextSize(2);
  _tft.setCursor(6, 4);
  _tft.print(F("Patient Monitor"));

  // Vital labels
  _tft.setTextSize(1);
  _tft.setTextColor(ILI9341_CYAN);
  _tft.setCursor(10, 26);
  _tft.print(F("TEMP C"));
  _tft.setTextColor(ILI9341_YELLOW);
  _tft.setCursor(115, 26);
  _tft.print(F("HR bpm"));
  _tft.setTextColor(ILI9341_MAGENTA);
  _tft.setCursor(220, 26);
  _tft.print(F("SpO2 %"));

  _tft.setTextColor(ILI9341_ORANGE);
  _tft.setCursor(10, 72);
  _tft.print(F("Pulse:"));

  // ECG graph frame
  _tft.drawRect(ECG_GRAPH_X - 1, ECG_GRAPH_Y - 1, ECG_GRAPH_W + 2, ECG_GRAPH_H + 2, ILI9341_DARKGREY);
  _tft.setTextColor(ILI9341_GREEN);
  _tft.setCursor(ECG_GRAPH_X, ECG_GRAPH_Y - 12);
  _tft.print(F("ECG LIVE"));

  // Center baseline guide
  const int midY = ECG_GRAPH_Y + (ECG_GRAPH_H / 2);
  for (int x = ECG_GRAPH_X; x < ECG_GRAPH_X + ECG_GRAPH_W; x += 6) {
    _tft.drawPixel(x, midY, ILI9341_DARKGREY);
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

  // Refresh Temp / HR / SpO2 / Pulse when values change (or every 500 ms)
  if (vitalsChanged(data) || (now - _lastVitalsMs) >= DISPLAY_REFRESH_INTERVAL_MS) {
    drawVitals(data);
    _lastVitalsMs = now;
  }

  if (statusChanged(data)) {
    drawStatusBar(data);
  }

  // Always plot next ECG sample for live scrolling graph
  plotEcgSample(data.ecgValue, data.ecgLeadOff);

  _lastData = data;
}

void TFTDisplay::drawVitals(const DisplayData &data) {
  // Clear value area only (keep labels)
  _tft.fillRect(0, 36, 320, 34, ILI9341_BLACK);
  _tft.fillRect(55, 70, 80, 14, ILI9341_BLACK);

  _tft.setTextSize(3);

  // Temperature
  _tft.setTextColor(ILI9341_CYAN);
  _tft.setCursor(8, 38);
  _tft.print(data.bodyTemperatureC, 1);

  // Heart rate (prefer MAX30102, else pulse)
  const int hr = (data.heartRateBpm > 0) ? data.heartRateBpm : data.pulseSensorBpm;
  _tft.setTextColor(ILI9341_YELLOW);
  _tft.setCursor(115, 38);
  _tft.print(hr);

  // SpO2
  _tft.setTextColor(ILI9341_MAGENTA);
  _tft.setCursor(220, 38);
  _tft.print(data.spo2Percent);

  // Pulse sensor BPM (small)
  _tft.setTextSize(1);
  _tft.setTextColor(ILI9341_ORANGE);
  _tft.setCursor(55, 72);
  _tft.print(data.pulseSensorBpm);
  _tft.print(F(" bpm"));
}

void TFTDisplay::drawStatusBar(const DisplayData &data) {
  _tft.fillRect(0, 205, 320, 35, ILI9341_DARKGREY);
  _tft.setTextSize(1);
  _tft.setTextColor(ILI9341_WHITE);

  _tft.setCursor(6, 210);
  _tft.print(F("GPS:"));
  _tft.print(data.gpsFix ? F("FIX ") : F("NO  "));
  if (data.gpsFix) {
    _tft.print(data.latitude, 4);
    _tft.print(',');
    _tft.print(data.longitude, 4);
  }

  _tft.setCursor(6, 224);
  _tft.print(F("GSM:"));
  _tft.print(data.gsmReady ? F("READY") : F("OFFLINE"));

  if (data.emergencyActive) {
    _tft.setTextColor(ILI9341_RED);
    _tft.setTextSize(2);
    _tft.setCursor(250, 212);
    _tft.print(F("SOS"));
  }
}

int16_t TFTDisplay::mapEcgToY(int ecgValue) const {
  // Map ADC around mid-scale into graph height
  const int mid = ADC_MID_SCALE;
  const int span = mid;  // +/- mid maps into full height
  int delta = ecgValue - mid;
  if (delta < -span) delta = -span;
  if (delta > span)  delta = span;

  // Invert Y (higher signal = higher on screen visually for ECG peaks up)
  const int y = ECG_GRAPH_Y + (ECG_GRAPH_H / 2) - ((delta * (ECG_GRAPH_H / 2)) / span);
  if (y < ECG_GRAPH_Y) return ECG_GRAPH_Y;
  if (y > ECG_GRAPH_Y + ECG_GRAPH_H - 1) return ECG_GRAPH_Y + ECG_GRAPH_H - 1;
  return (int16_t)y;
}

void TFTDisplay::plotEcgSample(int ecgValue, bool leadOff) {
  if (leadOff) {
    // Draw flat line + label once per wrap
    const int midY = ECG_GRAPH_Y + (ECG_GRAPH_H / 2);
    _tft.drawFastVLine(_ecgX, ECG_GRAPH_Y, ECG_GRAPH_H, ILI9341_BLACK);
    _tft.drawPixel(_ecgX, midY, ILI9341_RED);
    _lastEcgY = midY;

    if (_ecgX == ECG_GRAPH_X) {
      _tft.setTextSize(1);
      _tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
      _tft.setCursor(ECG_GRAPH_X + 90, ECG_GRAPH_Y + 4);
      _tft.print(F("LEAD OFF"));
    }
  } else {
    const int16_t y = mapEcgToY(ecgValue);

    // Erase column then draw segment
    _tft.drawFastVLine(_ecgX, ECG_GRAPH_Y, ECG_GRAPH_H, ILI9341_BLACK);

    if (_ecgX > ECG_GRAPH_X) {
      _tft.drawLine(_ecgX - 1, _lastEcgY, _ecgX, y, ILI9341_GREEN);
    } else {
      _tft.drawPixel(_ecgX, y, ILI9341_GREEN);
    }
    _lastEcgY = y;
  }

  // Moving erase cursor (gap) so waveform scrolls cleanly
  const int eraseX = _ecgX + 3;
  if (eraseX < ECG_GRAPH_X + ECG_GRAPH_W) {
    _tft.drawFastVLine(eraseX, ECG_GRAPH_Y, ECG_GRAPH_H, ILI9341_BLACK);
  }

  _ecgX++;
  if (_ecgX >= ECG_GRAPH_X + ECG_GRAPH_W) {
    _ecgX = ECG_GRAPH_X;
    // Redraw baseline dots after wrap
    const int midY = ECG_GRAPH_Y + (ECG_GRAPH_H / 2);
    for (int x = ECG_GRAPH_X; x < ECG_GRAPH_X + ECG_GRAPH_W; x += 8) {
      _tft.drawPixel(x, midY, ILI9341_DARKGREY);
    }
  }
}

bool TFTDisplay::vitalsChanged(const DisplayData &data) const {
  return (data.bodyTemperatureC != _lastData.bodyTemperatureC) ||
         (data.heartRateBpm      != _lastData.heartRateBpm) ||
         (data.pulseSensorBpm    != _lastData.pulseSensorBpm) ||
         (data.spo2Percent       != _lastData.spo2Percent);
}

bool TFTDisplay::statusChanged(const DisplayData &data) const {
  return (data.gpsFix != _lastData.gpsFix) ||
         (data.gsmReady != _lastData.gsmReady) ||
         (data.emergencyActive != _lastData.emergencyActive) ||
         (data.latitude != _lastData.latitude) ||
         (data.longitude != _lastData.longitude);
}
