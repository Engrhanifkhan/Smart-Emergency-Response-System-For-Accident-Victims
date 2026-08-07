#include "GSM_Module.h"
#include <string.h>
#include <ctype.h>

GSMModule gsmModule;

bool GSMModule::init() {
  clear();
#if defined(ESP32)
  GSM_SERIAL.begin(GSM_BAUD_RATE, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
#else
  GSM_SERIAL.begin(GSM_BAUD_RATE);
#endif
  delay(1000);

  if (!sendATCommand("AT", "OK", 2000)) {
    DEBUG_PRINTLN(F("[GSM] Modem not responding."));
    return false;
  }

  sendATCommand("ATE0", "OK", 2000);
  sendATCommand("AT+CMGF=1", "OK", 2000);          // text mode
  sendATCommand("AT+CNMI=2,2,0,0,0", "OK", 2000);  // new SMS direct to UART

  _data.networkState = GSM_STATE_INIT;
  _data.moduleReady  = true;
  DEBUG_PRINTLN(F("[GSM] Module initialized (SMS RX enabled)."));
  return true;
}

void GSMModule::update() {
  if (!_data.moduleReady) {
    return;
  }

  processIncoming();
  parseIncomingSmsBuffer();

  const unsigned long now = millis();
  if ((now - _lastHeartbeatMs) < GSM_HEARTBEAT_INTERVAL_MS) {
    return;
  }
  _lastHeartbeatMs = now;

  checkSignalStrength();

  if (_data.networkState == GSM_STATE_INIT) {
    if (sendATCommand("AT+CREG?", "+CREG: 0,1", 3000) ||
        sendATCommand("AT+CREG?", "+CREG: 0,5", 3000)) {
      _data.networkState = GSM_STATE_READY;
      DEBUG_PRINTLN(F("[GSM] Network registered."));
    }
  }
}

void GSMModule::clear() {
  _data.networkState   = GSM_STATE_OFFLINE;
  _data.signalStrength = 0;
  _data.moduleReady    = false;
  _data.lastSmsSuccess = false;
  _lastHeartbeatMs     = 0;
  _rxBuffer            = "";
  _lastSender[0]       = '\0';
  _lastBody[0]         = '\0';
  _smsPending          = false;
}

bool GSMModule::sendSMS(const char *phoneNumber, const char *message) {
  if (!_data.moduleReady || _data.networkState != GSM_STATE_READY) {
    DEBUG_PRINTLN(F("[GSM] Cannot send — network not ready."));
    return false;
  }

  _data.networkState = GSM_STATE_SENDING;
  _rxBuffer = "";

  GSM_SERIAL.print(F("AT+CMGS=\""));
  GSM_SERIAL.print(phoneNumber);
  GSM_SERIAL.println(F("\""));

  unsigned long start = millis();
  bool gotPrompt = false;
  while ((millis() - start) < 10000UL) {
    processIncoming();
    if (_rxBuffer.indexOf('>') >= 0) {
      gotPrompt = true;
      break;
    }
  }

  if (!gotPrompt) {
    DEBUG_PRINTLN(F("[GSM] No '>' prompt — SMS aborted."));
    _data.networkState = GSM_STATE_READY;
    return false;
  }

  GSM_SERIAL.print(message);
  GSM_SERIAL.write(26);

  _rxBuffer = "";
  start = millis();
  bool success = false;
  while ((millis() - start) < 30000UL) {
    processIncoming();
    if (_rxBuffer.indexOf(F("+CMGS:")) >= 0) {
      success = true;
      break;
    }
    if (_rxBuffer.indexOf(F("ERROR")) >= 0) {
      break;
    }
  }

  _data.lastSmsSuccess = success;
  _data.networkState = GSM_STATE_READY;

  DEBUG_PRINTLN(success ? F("[GSM] SMS sent.") : F("[GSM] SMS failed."));
  return success;
}

bool GSMModule::pollIncomingSms(char *senderOut, size_t senderLen,
                                char *bodyOut, size_t bodyLen) {
  processIncoming();
  parseIncomingSmsBuffer();

  if (!_smsPending) {
    return false;
  }

  if (senderOut != NULL && senderLen > 0) {
    strncpy(senderOut, _lastSender, senderLen - 1);
    senderOut[senderLen - 1] = '\0';
  }
  if (bodyOut != NULL && bodyLen > 0) {
    strncpy(bodyOut, _lastBody, bodyLen - 1);
    bodyOut[bodyLen - 1] = '\0';
  }

  _smsPending = false;
  return true;
}

const GSMData &GSMModule::getData() const {
  return _data;
}

bool GSMModule::sendATCommand(const char *command, const char *expected, uint16_t timeoutMs) {
  _rxBuffer = "";

  if (strlen(command) > 0) {
    GSM_SERIAL.println(command);
  }

  const unsigned long start = millis();
  while ((millis() - start) < timeoutMs) {
    processIncoming();
    if (_rxBuffer.indexOf(expected) >= 0) {
      return true;
    }
  }
  return false;
}

void GSMModule::processIncoming() {
  while (GSM_SERIAL.available() > 0) {
    const char c = (char)GSM_SERIAL.read();
    _rxBuffer += c;
    if (_rxBuffer.length() > 512) {
      _rxBuffer.remove(0, _rxBuffer.length() - 512);
    }
  }
}

void GSMModule::parseIncomingSmsBuffer() {
  // Format with CNMI=2,2,0,0,0:
  // +CMT: "+92300...","","yy/mm/dd,hh:mm:ss+zz"
  // STATUS
  int idx = _rxBuffer.indexOf("+CMT:");
  if (idx < 0) {
    return;
  }

  int lineEnd = _rxBuffer.indexOf('\n', idx);
  if (lineEnd < 0) {
    return; // wait for full header line
  }

  // Extract sender between first quotes
  int q1 = _rxBuffer.indexOf('"', idx);
  int q2 = (q1 >= 0) ? _rxBuffer.indexOf('"', q1 + 1) : -1;
  if (q1 >= 0 && q2 > q1) {
    String sender = _rxBuffer.substring(q1 + 1, q2);
    sender.toCharArray(_lastSender, sizeof(_lastSender));
  } else {
    _lastSender[0] = '\0';
  }

  // Body is the next line after header
  int bodyStart = lineEnd + 1;
  if (bodyStart >= (int)_rxBuffer.length()) {
    return; // wait for body
  }

  int bodyEnd = _rxBuffer.indexOf('\n', bodyStart);
  if (bodyEnd < 0) {
    // Accept if we already have some body text ending soon
    if ((millis() % 50) != 0) {
      // keep waiting a bit — use timeout by checking CR-only
    }
    bodyEnd = _rxBuffer.indexOf('\r', bodyStart);
    if (bodyEnd < 0 && (_rxBuffer.length() - bodyStart) < 2) {
      return;
    }
    if (bodyEnd < 0) {
      bodyEnd = _rxBuffer.length();
    }
  }

  String body = _rxBuffer.substring(bodyStart, bodyEnd);
  body.trim();
  body.toCharArray(_lastBody, sizeof(_lastBody));

  // Consume parsed SMS from buffer
  _rxBuffer.remove(0, bodyEnd > 0 ? bodyEnd : (_rxBuffer.length()));

  if (_lastBody[0] != '\0') {
    _smsPending = true;
    DEBUG_PRINT(F("[GSM] Incoming SMS: "));
    DEBUG_PRINTLN(_lastBody);
  }
}

void GSMModule::checkSignalStrength() {
  if (sendATCommand("AT+CSQ", "+CSQ:", 3000)) {
    const int idx = _rxBuffer.indexOf("+CSQ:");
    if (idx >= 0) {
      const int comma = _rxBuffer.indexOf(',', idx);
      if (comma > idx) {
        _data.signalStrength = _rxBuffer.substring(idx + 5, comma).toInt();
      }
    }
  }
}
