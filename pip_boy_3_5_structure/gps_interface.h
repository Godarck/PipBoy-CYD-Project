#ifndef GPS_INTERFACE_H
#define GPS_INTERFACE_H

#include <Arduino.h>

// Инициализация модуля (вызывать в setup)
void gpsInit();

// Проверка физической связи с GPS / C3-мостом / SC16IS750.
bool gpsCheckConnection();

// Обновление данных (вызывать в loop, желательно каждые 100-500 мс)
void gpsUpdate();

// --- Геттеры координат и статуса ---
bool     gpsHasFix();           // true если есть валидный fix
uint8_t  gpsGetSats();          // количество спутников
float    gpsGetLat();           // широта в градусах
float    gpsGetLon();           // долгота в градусах
float    gpsGetHdop();          // горизонтальная точность

// --- TinyGPS++ 1.1.0 fix quality ---
// 0=invalid, 1=GPS, 2=DGPS, 3=PPS, 4=RTK fixed, 5=RTK float, 6=Estimated, 7=Manual, 8=Simulated
uint8_t  gpsGetFixQuality();

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
