#ifndef COMMAND_MODULE_H
#define COMMAND_MODULE_H

#include "Config.h"

// ---------------------------------------------------------------------------
// Command Module — full ALL-MODULE report
// Serial / SMS: STATUS | DATA | CHECK | ALL | INFO | REPORT
// → Serial: complete dump of every module
// → SMS: 4 messages (vitals, ECG/pulse, GPS, GSM/emergency)
// ---------------------------------------------------------------------------

class CommandHandler {
public:
  bool init();
  void update();

private:
  char _serialLine[48];
  uint8_t _serialLen;

  bool isStatusCommand(const char *text) const;
  void handleCommand(const char *source, const char *replyPhone);
  void printFullReportToSerial() const;
  void sendFullSmsReport(const char *replyPhone) const;
  void normalizeCommand(char *text) const;
};

extern CommandHandler commandHandler;

#endif // COMMAND_MODULE_H
