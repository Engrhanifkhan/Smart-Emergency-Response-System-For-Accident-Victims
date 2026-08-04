#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include "Config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ---------------------------------------------------------------------------
// TFT Display — vitals (Temp/HR/SpO2) + live ECG scrolling graph
// ---------------------------------------------------------------------------

struct DisplayData {
  float bodyTemperatureC;
  int   heartRateBpm;      // MAX30102
  int   pulseSensorBpm;
  int   spo2Percent;
  int   ecgValue;
  bool  ecgLeadOff;
  bool  gpsFix;
  float latitude;
  float longitude;
  bool  gsmReady;
  bool  emergencyActive;
};

// Graph region (landscape 320x240)
#define ECG_GRAPH_X       4
#define ECG_GRAPH_Y       95
#define ECG_GRAPH_W       312
#define ECG_GRAPH_H       105

class TFTDisplay {
public:
  bool init();
  void update(const DisplayData &data);
  void showBootScreen();
  void showMessage(const char *title, const char *message, uint16_t color);
  void showHardwareTestScreen();
  void clear();

private:
#if defined(ESP32)
  Adafruit_ILI9341 _tft{TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN};
#else
  Adafruit_ILI9341 _tft{TFT_CS_PIN, TFT_DC_PIN, TFT_MOSI_PIN, TFT_SCK_PIN, TFT_RST_PIN, TFT_MISO_PIN};
#endif
  unsigned long _lastVitalsMs;
  DisplayData   _lastData;
  bool          _layoutReady;
  int16_t       _ecgX;
  int16_t       _lastEcgY;

  void drawStaticLayout();
  void drawVitals(const DisplayData &data);
  void drawStatusBar(const DisplayData &data);
  void plotEcgSample(int ecgValue, bool leadOff);
  int16_t mapEcgToY(int ecgValue) const;
  bool vitalsChanged(const DisplayData &data) const;
  bool statusChanged(const DisplayData &data) const;
};

extern TFTDisplay tftDisplay;

#endif // TFT_DISPLAY_H
