#ifndef GSM_MODULE_H
#define GSM_MODULE_H

#include "Config.h"

// ---------------------------------------------------------------------------
// GSM Module — SIM800L AT commands + SMS send/receive
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
  // Returns true once when a new SMS body is available (command check)
  bool pollIncomingSms(char *senderOut, size_t senderLen,
                       char *bodyOut, size_t bodyLen);
  const GSMData &getData() const;

private:
  GSMData       _data;
  unsigned long _lastHeartbeatMs;
  String        _rxBuffer;

  char _lastSender[24];
  char _lastBody[96];
  bool _smsPending;

  bool sendATCommand(const char *command, const char *expected, uint16_t timeoutMs);
  void processIncoming();
  void checkSignalStrength();
  void parseIncomingSmsBuffer();
};

extern GSMModule gsmModule;

#endif // GSM_MODULE_H
