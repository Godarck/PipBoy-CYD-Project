#include "tof10120.h"

TOF10120Sensor::TOF10120Sensor() 
    : lastRawDistance(TOF10120_ERROR_VALUE), 
      lastReadTime(0),
      filterIndex(0),
      filterCount(0)
{
    // Инициализируем буфер нулями
    for (int i = 0; i < TOF10120_FILTER_WINDOW; i++) {
        filterBuffer[i] = 0;
    }
}

bool TOF10120Sensor::isAvailable() {
    Wire.beginTransmission(TOF10120_I2C_ADDR);
    return (Wire.endTransmission() == 0);
}

bool TOF10120Sensor::readRaw(uint8_t* buffer, uint8_t len) {
    // Шаг 1: устанавливаем указатель на регистр 0x00
    Wire.beginTransmission(TOF10120_I2C_ADDR);
    Wire.write(0x00);               // адрес регистра расстояния
    if (Wire.endTransmission() != 0) {
        return false;
    }

    // Даташит: минимум 30 мкс после записи адреса
    delayMicroseconds(100);

    // Шаг 2: читаем 2 байта
    uint8_t received = Wire.requestFrom(TOF10120_I2C_ADDR, len);
    if (received != len) {
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        if (Wire.available()) {
            buffer[i] = Wire.read();
        } else {
            return false;
        }
    }
    return true;
}

bool TOF10120Sensor::isValueValid(int value) const {
    if (value == TOF10120_ERROR_VALUE) return false;
    if (value == 0) return false;              // датчик ничего не видит
    if (value > TOF10120_MAX_DISTANCE) return false; // > 2000 мм — ошибка
    if (value < TOF10120_MIN_DISTANCE) return false;
    return true;
}

void TOF10120Sensor::pushToFilter(int value) {
    filterBuffer[filterIndex] = value;
    filterIndex++;
    if (filterIndex >= TOF10120_FILTER_WINDOW) {
        filterIndex = 0;
    }
    if (filterCount < TOF10120_FILTER_WINDOW) {
        filterCount++;
    }
}

int TOF10120Sensor::calculateAverage() const {
    if (filterCount == 0) {
        return TOF10120_ERROR_VALUE;
    }
    long sum = 0;
    for (uint8_t i = 0; i < filterCount; i++) {
        sum += filterBuffer[i];
    }
    return (int)(sum / filterCount);
}

int TOF10120Sensor::readDistanceRaw() {
    uint8_t buf[2];
    if (!readRaw(buf, 2)) {
        lastRawDistance = TOF10120_ERROR_VALUE;
        return TOF10120_ERROR_VALUE;
    }

    // Собираем 16-битное значение: старший + младший байт
    uint16_t dist = ((uint16_t)buf[0] << 8) | buf[1];

    // Если датчик ничего не видит, часто выдает 0 или 0xFFFF
    if (dist == 0xFFFF || dist == 0) {
        lastRawDistance = TOF10120_ERROR_VALUE;
        return TOF10120_ERROR_VALUE;
    }

    lastRawDistance = (int)dist;
    lastReadTime = millis();
    return lastRawDistance;
}

int TOF10120Sensor::readDistanceFiltered() {
    int raw = readDistanceRaw();

    if (isValueValid(raw)) {
        pushToFilter(raw);
        return calculateAverage();
    }

    // Значение невалидное (в т.ч. > 2000) — не кладем в буфер,
    // возвращаем текущее среднее, если оно есть
    if (filterCount > 0) {
        return calculateAverage();
    }

    return TOF10120_ERROR_VALUE;
}

int TOF10120Sensor::getFilteredDistance() const {
    return calculateAverage();
}

void TOF10120Sensor::resetFilter() {
    filterIndex = 0;
    filterCount = 0;
    for (int i = 0; i < TOF10120_FILTER_WINDOW; i++) {
        filterBuffer[i] = 0;
    }
}

int TOF10120Sensor::getLastRawDistance() const {
    return lastRawDistance;
}

unsigned long TOF10120Sensor::getLastReadTime() const {
    return lastReadTime;
}

bool TOF10120Sensor::isDataFresh(unsigned long maxAgeMs) const {
    if (lastReadTime == 0) return false;
    return (millis() - lastReadTime) < maxAgeMs;
}

// Глобальный экземпляр
TOF10120Sensor tofSensor;
