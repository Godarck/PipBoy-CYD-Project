#include "BMP280Module.h"
#include "config.h"

Adafruit_BMP280 bmp;
Adafruit_BME280 bme;

SensorType activeSensor = SENSOR_NONE;

static bool _available = false;
static bool _calibrated = false;
static float _lastTemp = 0.0f;
static float _lastPress = 0.0f;
static float _lastHum = 0.0f;
static float _groundPressure = 0.0f;
static float _groundTemp = 0.0f;
static float _seaLevelPressure = 1013.25f;

static constexpr float MMHG_PER_HPA = 0.7500637554192f;
static constexpr float KPA_PER_HPA = 0.1f;
static constexpr float GAS_CONSTANT = 287.05f;
static constexpr float GRAVITY = 9.80665f;

// ============================================================
// Чтение ID через Wire (до инициализации библиотеки)
// ============================================================
static uint8_t readSensorID(uint8_t addr) {
    Wire.beginTransmission(addr);
    Wire.write(0xD0); // Регистр ID
    if (Wire.endTransmission() != 0) return 0x00;
    
    if (DEBUGFLAG) Serial.println("[WIRE] Read BMP/BME Sensor ID");
    Wire.requestFrom((int)addr, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0x00;
}

// ============================================================
// Инициализация с автоопределением
// ============================================================
bool bmpInit() {
    // Пробуем BMP280 (ID = 0x58)
    uint8_t id = readSensorID(BMP280_ADDRESS_ALT); // 0x77
    if (id == 0x58) {
        if (bmp.begin(BMP280_ADDRESS)) {
            activeSensor = SENSOR_BMP280;
            _available = true;
            
            if (DEBUGFLAG) Serial.println("[BMP] Found BMP280");
            bmp.setSampling(
                Adafruit_BMP280::MODE_FORCED,
                Adafruit_BMP280::SAMPLING_X2,
                Adafruit_BMP280::SAMPLING_X16,
                Adafruit_BMP280::FILTER_X16,
                Adafruit_BMP280::STANDBY_MS_500
            );
            return true;
        }
    }
    
    // Пробуем BME280 (ID = 0x60)
    if (id == 0x60 || readSensorID(0x76) == 0x60) { // Проверяем оба адреса
        uint8_t addr = (id == 0x60) ? BMP280_ADDRESS_ALT : 0x76;
        if (bme.begin(addr)) {
            activeSensor = SENSOR_BME280;
            _available = true;
            
            if (DEBUGFLAG) Serial.println("[BMP] Found BME280");
            bme.setSampling(
                Adafruit_BME280::MODE_FORCED,
                Adafruit_BME280::SAMPLING_X2,
                Adafruit_BME280::SAMPLING_X16,
                Adafruit_BME280::SAMPLING_X1,  // Влажность
                Adafruit_BME280::FILTER_X16,
                Adafruit_BME280::STANDBY_MS_500
            );
            return true;
        }
    }
    
    if (DEBUGFLAG) Serial.println("[BMP] None supported sensor found!");
    // Ничего не найдено
    activeSensor = SENSOR_NONE;
    _available = false;
    return false;
}

// ============================================================
// Измерение — универсальное
// ============================================================
bool bmpMeasure() {
    if (!_available) return false;
    
    if (activeSensor == SENSOR_BMP280) {
        if (!bmp.takeForcedMeasurement()) return false;
        _lastTemp = bmp.readTemperature();
        _lastPress = bmp.readPressure() / 100.0f;
        _lastHum = 0.0f;
        return true;
        
    } else if (activeSensor == SENSOR_BME280) {
        if (!bme.takeForcedMeasurement()) return false;
        _lastTemp = bme.readTemperature();
        _lastPress = bme.readPressure() / 100.0f;
        _lastHum = bme.readHumidity();
        return true;
    }
    
    return false;
}

// ============================================================
// Геттеры
// ============================================================
float bmpGetPressureHpa()  { return _lastPress; }
float bmpGetPressureKpa()  { return _lastPress * KPA_PER_HPA; }
float bmpGetPressureMmHg() { return _lastPress * MMHG_PER_HPA; }
float bmpGetTemperatureC() { return _lastTemp; }
float bmpGetTemperatureF() { return _lastTemp * 9.0f/5.0f + 32.0f; }
float bmpGetHumidity()     { return _lastHum; } // 0 для BMP280

// ============================================================
// Высота — одинаковая формула для обоих
// ============================================================
void bmpCalibrateAltitude() {
    if (bmpMeasure()) {
        _groundPressure = _lastPress;
        _groundTemp = _lastTemp;
        _calibrated = true;
    }
}

bool bmpIsCalibrated() { return _calibrated; }

static float calcAltitude(float p0, float p, float t) {
    if (p <= 0 || p0 <= 0) return 0.0f;
    float T = t + 273.15f;
    return (GAS_CONSTANT * T / GRAVITY) * logf(p0 / p);
}

float bmpGetRelativeAltitude() {
    if (!_calibrated) return 0.0f;
    return calcAltitude(_groundPressure, _lastPress, _lastTemp);
}

float bmpGetSeaLevelAltitude() {
    return calcAltitude(_seaLevelPressure, _lastPress, _lastTemp);
}

void bmpSetSeaLevelPressure(float hpa) {
    _seaLevelPressure = hpa;
}
