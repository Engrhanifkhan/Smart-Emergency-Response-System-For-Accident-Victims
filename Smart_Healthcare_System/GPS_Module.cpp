#include "GPS_Module.h"
#include <string.h>
#include <stdlib.h>

GPSModule gpsModule;

bool GPSModule::init() {
  clear();
#if defined(ESP32)
  GPS_SERIAL.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#else
  GPS_SERIAL.begin(GPS_BAUD_RATE);
#endif
  _data.moduleReady = true;
  DEBUG_PRINTLN(F("[GPS] Module initialized."));
  return true;
}

void GPSModule::update() {
  if (!_data.moduleReady) {
    return;
  }

  while (GPS_SERIAL.available() > 0) {
    processSerialByte((char)GPS_SERIAL.read());
  }

  const unsigned long now = millis();
  if (_data.fixValid && (now - _lastUpdateMs) > 5000UL) {
    // Invalidate fix if no fresh data received within timeout.
    _data.fixValid = false;
  }
}

void GPSModule::clear() {
  _data.latitude    = 0.0f;
  _data.longitude   = 0.0f;
  _data.altitudeM   = 0.0f;
  _data.satellites  = 0;
  _data.fixValid    = false;
  _data.moduleReady = false;
  _lastUpdateMs     = 0;
  _nmeaIndex        = 0;
  memset(_nmeaBuffer, 0, sizeof(_nmeaBuffer));
}

const GPSData &GPSModule::getData() const {
  return _data;
}

void GPSModule::processSerialByte(char c) {
  if (c == '$') {
    _nmeaIndex = 0;
  }

  if (_nmeaIndex < (sizeof(_nmeaBuffer) - 1)) {
    _nmeaBuffer[_nmeaIndex++] = c;
    _nmeaBuffer[_nmeaIndex] = '\0';
  }

  if (c == '\n') {
    if (strncmp(_nmeaBuffer, "$GPGGA", 6) == 0 ||
        strncmp(_nmeaBuffer, "$GNGGA", 6) == 0) {
      parseGPGGA(_nmeaBuffer);
    }
    _nmeaIndex = 0;
  }
}

void GPSModule::parseGPGGA(const char *sentence) {
  char buffer[82];
  strncpy(buffer, sentence, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  char *token = strtok(buffer, ",");
  int field = 0;
  char lat[16] = {0};
  char latDir = 'N';
  char lon[16] = {0};
  char lonDir = 'E';
  char sats[4] = {0};
  char alt[12] = {0};
  int fixQuality = 0;

  while (token != NULL) {
    switch (field) {
      case 2:  fixQuality = atoi(token); break;
      case 3:  strncpy(lat, token, sizeof(lat) - 1); break;
      case 4:  latDir = token[0]; break;
      case 5:  strncpy(lon, token, sizeof(lon) - 1); break;
      case 6:  lonDir = token[0]; break;
      case 7:  strncpy(sats, token, sizeof(sats) - 1); break;
      case 9:  strncpy(alt, token, sizeof(alt) - 1); break;
      default: break;
    }
    token = strtok(NULL, ",");
    ++field;
  }

  _data.fixValid   = (fixQuality > 0);
  _data.satellites = atoi(sats);
  _data.altitudeM  = atof(alt);

  if (_data.fixValid) {
    _data.latitude  = parseCoordinate(lat, &latDir);
    _data.longitude = parseCoordinate(lon, &lonDir);
    _lastUpdateMs   = millis();
  }
}

float GPSModule::parseCoordinate(const char *value, const char *direction) {
  if (strlen(value) < 4) {
    return 0.0f;
  }

  const float raw = atof(value);
  const int degrees = (int)(raw / 100.0f);
  const float minutes = raw - (degrees * 100.0f);
  float decimal = degrees + (minutes / 60.0f);

  if (*direction == 'S' || *direction == 'W') {
    decimal = -decimal;
  }
  return decimal;
}
