#ifndef BMP280_MODULE_H
#define BMP280_MODULE_H

#include <Adafruit_BMP280.h>

// Глобальный объект датчика
extern Adafruit_BMP280 bmp;

// Инициализация (возвращает true если найден)
bool bmpInit();

// Измерение (forced mode — запуск и ожидание)
bool bmpMeasure();

// Геттеры последнего измерения
float bmpGetPressureHpa();
float bmpGetPressureKpa();
float bmpGetPressureMmHg();
float bmpGetTemperatureC();
float bmpGetTemperatureF();

// --- Высота ---
void bmpCalibrateAltitude();      // Запомнить текущее как "ноль"
bool bmpIsCalibrated();
float bmpGetRelativeAltitude();   // От калиброванной точки
float bmpGetSeaLevelAltitude();   // От уровня моря
void bmpSetSeaLevelPressure(float hpa); // Установить P моря (метео)

#endif
