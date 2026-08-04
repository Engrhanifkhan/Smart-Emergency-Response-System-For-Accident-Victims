#include "GSM_Module.h"

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

  sendATCommand("ATE0", "OK", 2000);   // Disable echo
  sendATCommand("AT+CMGF=1", "OK", 2000); // SMS text mode
  sendATCommand("AT+CNMI=1,2,0,0,0", "OK", 2000);

  _data.networkState = GSM_STATE_INIT;
  _data.moduleReady  = true;
  DEBUG_PRINTLN(F("[GSM] Module initialized."));
  return true;
}

void GSMModule::update() {
  if (!_data.moduleReady) {
    return;
  }

  processIncoming();

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

  // Wait for ">" prompt before sending message body
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
  GSM_SERIAL.write(26); // Ctrl+Z to send

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
    if (_rxBuffer.length() > 256) {
      _rxBuffer.remove(0, _rxBuffer.length() - 256);
    }
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
