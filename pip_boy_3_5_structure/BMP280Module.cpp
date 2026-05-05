#include "BMP280Module.h"
#include <Adafruit_BMP280.h>
#include <config.h>

Adafruit_BMP280 bmp;

static bool _available = false;
static bool _calibrated = false;
static float _lastTemp = 0.0f;
static float _lastPress = 0.0f;     // hPa
static float _groundPressure = 0.0f;
static float _groundTemp = 0.0f;
static float _seaLevelPressure = 1013.25f;

static constexpr float MMHG_PER_HPA = 0.7500637554192f;
static constexpr float KPA_PER_HPA = 0.1f;
static constexpr float GAS_CONSTANT = 287.05f;
static constexpr float GRAVITY = 9.80665f;

bool bmpInit() {

    //Wire.begin(RTC_SDA, RTC_SCL);
    if (!bmp.begin(BMP280_ADDRESS)) 
    {
        _available = false;
        return false;
    }
    _available = true;
    
    bmp.setSampling(
        Adafruit_BMP280::MODE_FORCED,
        Adafruit_BMP280::SAMPLING_X2,
        Adafruit_BMP280::SAMPLING_X16,
        Adafruit_BMP280::FILTER_X16,
        Adafruit_BMP280::STANDBY_MS_500
    );
    
    return true;
}

bool bmpMeasure() {
    if (!_available) return false;
    if (!bmp.takeForcedMeasurement()) return false;
    
    _lastTemp = bmp.readTemperature();
    _lastPress = bmp.readPressure() / 100.0f;
    return true;
}

float bmpGetPressureHpa()  { return _lastPress; }
float bmpGetPressureKpa()  { return _lastPress * KPA_PER_HPA; }
float bmpGetPressureMmHg() { return _lastPress * MMHG_PER_HPA; }
float bmpGetTemperatureC() { return _lastTemp; }
float bmpGetTemperatureF() { return _lastTemp * 9.0f/5.0f + 32.0f; }

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
