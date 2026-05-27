#ifndef GPS_INTERFACE_H
#define GPS_INTERFACE_H

#include <Arduino.h>

// Инициализация модуля (вызывать в setup)
void gpsInit();

// Проверка физической связи с GPS UART / C3-мостом / SC16IS750.
bool gpsCheckConnection();

// Обновление данных (вызывать в loop, желательно каждые 100-500 мс)
void gpsUpdate();

// --- Геттеры координат и статуса ---
bool     gpsHasFix();           // true если есть валидный fix
uint8_t  gpsGetSats();          // количество спутников
float    gpsGetLat();           // широта в градусах
float    gpsGetLon();           // долгота в градусах
float    gpsGetHdop();          // горизонтальная точность

// --- TinyGPS++ 1.1.0 новые методы ---
uint8_t  gpsGetFixQuality();    // 0=invalid, 1=GPS, 2=DGPS, 4=RTK fixed, 5=RTK float...
uint8_t  gpsGetFixMode();       // 1=No fix, 2=2D, 3=3D

// --- Движение и высота ---
float    gpsGetSpeedKmph();     // скорость в км/ч
float    gpsGetAltitude();      // высота в метрах над уровнем моря
float    gpsGetCourse();        // курс (направление движения) в градусах 0-360

// --- Время и дата ---
uint8_t  gpsGetHour();          // UTC часы
uint8_t  gpsGetMinute();        // UTC минуты
uint8_t  gpsGetSecond();        // UTC секунды
uint16_t gpsGetYear();          // год
uint8_t  gpsGetMonth();         // месяц
uint8_t  gpsGetDay();           // день

// --- LED управление (только для I2C-C3, в остальных режимах — заглушки) ---
void gpsSetLedColor(uint8_t r, uint8_t g, uint8_t b);
void gpsSetLedAuto();

#endif
