#ifndef BMP280_MODULE_H
#define BMP280_MODULE_H

#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <Wire.h>

// Тип датчика
enum SensorType {
    SENSOR_NONE = 0,
    SENSOR_BMP280,
    SENSOR_BME280
};

// Глобальные объекты (только один будет инициализирован)
extern Adafruit_BMP280 bmp;
extern Adafruit_BME280 bme;

// Текущий активный датчик
extern SensorType activeSensor;

// Инициализация с автоопределением
bool bmpInit();

// Измерение (универсальное для обоих датчиков)
bool bmpMeasure();

// Геттеры
float bmpGetPressureHpa();
float bmpGetPressureKpa();
float bmpGetPressureMmHg();
float bmpGetTemperatureC();
float bmpGetTemperatureF();
float bmpGetHumidity();           // Только для BME280, для BMP280 вернёт 0

// Высота
void bmpCalibrateAltitude();
bool bmpIsCalibrated();
float bmpGetRelativeAltitude();
float bmpGetSeaLevelAltitude();
void bmpSetSeaLevelPressure(float hpa);

#endif
