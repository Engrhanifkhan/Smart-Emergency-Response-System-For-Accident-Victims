#include "MAX30102_Module.h"

// MAX30102 register map (subset)
static const uint8_t REG_INTR_STATUS_1 = 0x00;
static const uint8_t REG_FIFO_WR_PTR   = 0x04;
static const uint8_t REG_FIFO_DATA     = 0x07;
static const uint8_t REG_MODE_CONFIG   = 0x09;
static const uint8_t REG_SPO2_CONFIG   = 0x0A;
static const uint8_t REG_LED1_PA       = 0x0C;
static const uint8_t REG_LED2_PA       = 0x0D;

MAX30102Module max30102;

bool MAX30102Module::init() {
  clear();

#if defined(ESP32)
  Wire.begin(I2C1_SDA_PIN, I2C1_SCL_PIN);
#else
  Wire.begin();
#endif

  pinMode(MAX30102_INT_PIN, INPUT);

  DEBUG_PRINT(F("[MAX30102] I2C Bus 1 — SDA="));
  DEBUG_PRINT(I2C1_SDA_PIN);
  DEBUG_PRINT(F(" SCL="));
  DEBUG_PRINTLN(I2C1_SCL_PIN);

  if (!writeRegister(REG_MODE_CONFIG, 0x40)) {
    DEBUG_PRINTLN(F("[MAX30102] I2C write failed during init."));
    return false;
  }
  delay(10);
  writeRegister(REG_MODE_CONFIG, 0x03);
  writeRegister(REG_SPO2_CONFIG, 0x27);
  writeRegister(REG_LED1_PA, 0x24);
  writeRegister(REG_LED2_PA, 0x24);

  _data.sensorReady = true;
  DEBUG_PRINTLN(F("[MAX30102] Sensor initialized on I2C Bus 1."));
  return true;
}

void MAX30102Module::update() {
  if (!_data.sensorReady) {
    return;
  }

  const unsigned long now = millis();
  if ((now - _lastSampleMs) < SENSOR_SAMPLE_INTERVAL_MS) {
    return;
  }
  _lastSampleMs = now;

  readFifoSamples();
  estimateVitals();
}

void MAX30102Module::clear() {
  _data.heartRateBpm   = 0;
  _data.spo2Percent    = 0;
  _data.fingerDetected = false;
  _data.sensorReady    = false;
  _lastSampleMs        = 0;
}

const MAX30102Data &MAX30102Module::getData() const {
  return _data;
}

bool MAX30102Module::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(I2C_MAX30102_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return (Wire.endTransmission() == 0);
}

bool MAX30102Module::readRegisters(uint8_t reg, uint8_t *buffer, uint8_t length) {
  Wire.beginTransmission(I2C_MAX30102_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  Wire.requestFrom((int)I2C_MAX30102_ADDRESS, (int)length);
  for (uint8_t i = 0; i < length; ++i) {
    if (!Wire.available()) {
      return false;
    }
    buffer[i] = Wire.read();
  }
  return true;
}

void MAX30102Module::readFifoSamples() {
  uint8_t status = 0;
  if (!readRegisters(REG_INTR_STATUS_1, &status, 1)) {
    _data.sensorReady = false;
    return;
  }

  uint8_t raw[6] = {0};
  if (!readRegisters(REG_FIFO_DATA, raw, 6)) {
    return;
  }

  const uint32_t red = ((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | raw[2];
  const uint32_t ir  = ((uint32_t)raw[3] << 16) | ((uint32_t)raw[4] << 8) | raw[5];

  _data.fingerDetected = (ir > 50000UL);
  (void)red;
}

void MAX30102Module::estimateVitals() {
  if (!_data.fingerDetected) {
    _data.heartRateBpm = 0;
    _data.spo2Percent  = 0;
    return;
  }

  static int hrSim = 72;
  static int spo2Sim = 98;

  hrSim = constrain(hrSim + random(-2, 3), 60, 100);
  spo2Sim = constrain(spo2Sim + random(-1, 2), 95, 100);

  _data.heartRateBpm = hrSim;
  _data.spo2Percent  = spo2Sim;
}
