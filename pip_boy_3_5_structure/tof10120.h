#ifndef TOF10120_H
#define TOF10120_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class TOF10120Sensor {
public:
    TOF10120Sensor();

    // Проверяет, отвечает ли датчик на шине
    bool isAvailable();

    // Читает "сырое" расстояние в мм (без фильтра). При ошибке возвращает TOF10120_ERROR_VALUE
    int readDistanceRaw();

    // Читает расстояние со скользящим средним.
    // Значения > TOF10120_MAX_DISTANCE считаются ошибкой и не попадают в буфер.
    // При ошибке возвращает текущее среднее (если буфер не пуст) или TOF10120_ERROR_VALUE
    int readDistanceFiltered();

    // Получить текущее среднее значение без нового чтения
    int getFilteredDistance() const;

    // Сбросить буфер фильтра
    void resetFilter();

    // Последнее успешно прочитанное сырые значение
    int getLastRawDistance() const;

    // Время последнего успешного чтения (millis)
    unsigned long getLastReadTime() const;

    // Проверяет, не устарели ли данные
    bool isDataFresh(unsigned long maxAgeMs = 1000) const;

private:
    int lastRawDistance;
    unsigned long lastReadTime;

    // Буфер скользящего среднего
    int filterBuffer[TOF10120_FILTER_WINDOW];
    uint8_t filterIndex;
    uint8_t filterCount;

    bool readRaw(uint8_t* buffer, uint8_t len);
    bool isValueValid(int value) const;
    void pushToFilter(int value);
    int calculateAverage() const;
};

// Глобальный экземпляр для удобства
extern TOF10120Sensor tofSensor;

#endif // TOF10120_H
