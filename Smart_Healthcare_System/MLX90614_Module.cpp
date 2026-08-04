#include "MLX90614_Module.h"

#if defined(ESP32)
  #define TEMP_I2C Wire1   // I2C Bus 2 — separate from MAX30102
#else
  #define TEMP_I2C Wire    // Mega: single hardware I2C (shared pins 20/21)
#endif

static const uint8_t REG_TA = 0x06;
static const uint8_t REG_TO = 0x07;

MLX90614Module mlx90614;

bool MLX90614Module::init() {
  clear();

#if defined(ESP32)
  Wire1.begin(I2C2_SDA_PIN, I2C2_SCL_PIN);
  DEBUG_PRINT(F("[MLX90614] I2C Bus 2 — SDA="));
  DEBUG_PRINT(I2C2_SDA_PIN);
  DEBUG_PRINT(F(" SCL="));
  DEBUG_PRINTLN(I2C2_SCL_PIN);
#else
  Wire.begin();
  DEBUG_PRINTLN(F("[MLX90614] I2C Bus 2 — using Mega Wire (pins 20/21)"));
#endif

  TEMP_I2C.beginTransmission(I2C_MLX90614_ADDRESS);
  if (TEMP_I2C.endTransmission() != 0) {
    DEBUG_PRINTLN(F("[MLX90614] Sensor not detected on I2C Bus 2."));
    return false;
  }

  _data.sensorReady = true;
  DEBUG_PRINTLN(F("[MLX90614] Sensor initialized on I2C Bus 2."));
  return true;
}

void MLX90614Module::update() {
  if (!_data.sensorReady) {
    return;
  }

  const unsigned long now = millis();
  if ((now - _lastSampleMs) < SENSOR_SAMPLE_INTERVAL_MS) {
    return;
  }
  _lastSampleMs = now;

  _data.ambientTemperatureC = readTemperature(REG_TA);
  _data.objectTemperatureC  = readTemperature(REG_TO);
}

void MLX90614Module::clear() {
  _data.ambientTemperatureC = 0.0f;
  _data.objectTemperatureC  = 0.0f;
  _data.sensorReady         = false;
  _lastSampleMs             = 0;
}

const MLX90614Data &MLX90614Module::getData() const {
  return _data;
}

float MLX90614Module::readTemperature(uint8_t reg) {
  TEMP_I2C.beginTransmission(I2C_MLX90614_ADDRESS);
  TEMP_I2C.write(reg);
  if (TEMP_I2C.endTransmission(false) != 0) {
    _data.sensorReady = false;
    return 0.0f;
  }

  TEMP_I2C.requestFrom((int)I2C_MLX90614_ADDRESS, 3);
  if (TEMP_I2C.available() < 3) {
    return 0.0f;
  }

  const uint8_t low  = TEMP_I2C.read();
  const uint8_t high = TEMP_I2C.read();
  const uint8_t pec  = TEMP_I2C.read();
  (void)pec;

  const uint16_t raw = (uint16_t)high << 8 | low;
  return raw * 0.02f - 273.15f;
}
