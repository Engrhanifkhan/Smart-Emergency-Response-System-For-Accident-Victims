#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include "Config.h"

// ---------------------------------------------------------------------------
// GPS Module
// Parses NMEA sentences from a NEO-6M (or compatible) GPS receiver
// connected to Hardware Serial2 (RX2=D17, TX2=D16).
// ---------------------------------------------------------------------------

struct GPSData {
  float latitude;
  float longitude;
  float altitudeM;
  int   satellites;
  bool  fixValid;
  bool  moduleReady;
};

class GPSModule {
public:
  bool init();
  void update();
  void clear();

  const GPSData &getData() const;

private:
  GPSData       _data;
  unsigned long _lastUpdateMs;
  char          _nmeaBuffer[82];
  uint8_t       _nmeaIndex;

  void processSerialByte(char c);
  void parseGPGGA(const char *sentence);
  float parseCoordinate(const char *value, const char *direction);
};

extern GPSModule gpsModule;

#endif // GPS_MODULE_H
