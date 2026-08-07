#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include "Config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_HX8357.h>

// ---------------------------------------------------------------------------
// TFT Display — HX8357D 480x320
// Display ALWAYS works. Missing sensors show "--" / OFF.
// ---------------------------------------------------------------------------

#define HX8357_NAVY       0x000F
#define HX8357_DARKGREEN  0x03E0
#define HX8357_DARKGREY   0x7BEF
#define HX8357_ORANGE     0xFD20

struct DisplayData {
  float bodyTemperatureC;
  int   heartRateBpm;
  int   pulseSensorBpm;
  int   pulseRaw;
  int   spo2Percent;
  int   ecgValue;
  int   ecgBaseline;
  bool  ecgLeadOff;
  bool  gpsFix;
  float latitude;
  float longitude;
  bool  gsmReady;
  bool  emergencyActive;

  // Module online flags (false = not connected / init failed)
  bool  tempOk;
  bool  max30102Ok;
  bool  pulseOk;
  bool  ecgOk;
  bool  gpsOk;
  bool  gsmOk;
};

#define TFT_W             480
#define TFT_H             320
#define ECG_GRAPH_X       6
#define ECG_GRAPH_Y       132
#define ECG_GRAPH_W       468
#define ECG_GRAPH_H       132

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
  Adafruit_HX8357 _tft{TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN};
#else
  Adafruit_HX8357 _tft{TFT_CS_PIN, TFT_DC_PIN, TFT_MOSI_PIN, TFT_SCK_PIN, TFT_RST_PIN, TFT_MISO_PIN};
#endif
  unsigned long _lastVitalsMs;
  DisplayData   _lastData;
  bool          _layoutReady;
  int16_t       _ecgX;
  int16_t       _lastEcgY;

  void drawStaticLayout();
  void drawVitals(const DisplayData &data);
  void drawStatusBar(const DisplayData &data);
  void plotEcgSample(int ecgValue, int baseline, bool leadOff, bool ecgOk);
  int16_t mapEcgToY(int ecgValue, int baseline) const;
  bool vitalsChanged(const DisplayData &data) const;
  bool statusChanged(const DisplayData &data) const;
};

extern TFTDisplay tftDisplay;

#endif // TFT_DISPLAY_H
