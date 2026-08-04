#ifndef GSM_MODULE_H
#define GSM_MODULE_H

#include "Config.h"

// ---------------------------------------------------------------------------
// GSM Module
// Controls a SIM800L (or compatible) modem via AT commands on Serial1
// (RX1=D19, TX1=D18). Use an external 4.0V supply for SIM800L VCC.
// Supports SMS alerts and network readiness checks.
// ---------------------------------------------------------------------------

enum GSMNetworkState {
  GSM_STATE_OFFLINE = 0,
  GSM_STATE_INIT,
  GSM_STATE_READY,
  GSM_STATE_SENDING
};

struct GSMData {
  GSMNetworkState networkState;
  int             signalStrength;
  bool            moduleReady;
  bool            lastSmsSuccess;
};

class GSMModule {
public:
  bool init();
  void update();
  void clear();

  bool sendSMS(const char *phoneNumber, const char *message);
  const GSMData &getData() const;

private:
  GSMData       _data;
  unsigned long _lastHeartbeatMs;
  String        _rxBuffer;

  bool sendATCommand(const char *command, const char *expected, uint16_t timeoutMs);
  void processIncoming();
  void checkSignalStrength();
};

extern GSMModule gsmModule;

#endif // GSM_MODULE_H
