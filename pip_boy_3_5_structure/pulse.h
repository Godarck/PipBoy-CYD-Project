#ifndef PULSE_H
#define PULSE_H

#include <Arduino.h>
#include "config.h"

enum PulseStatus {
    PULSE_OK = 0,
    PULSE_SENSOR_NOT_FOUND,
    PULSE_FINGER_NOT_DETECTED,
    PULSE_CALIBRATING
};

struct PulseData {
    int bpm;              // Удары в минуту
    int spo2;             // SpO2 %
    bool fingerDetected;  // Палец приложен?
    float temperaure;
    PulseStatus status;   // Текущий статус
};

// --- Флаг физического подключения ---
// true = датчик отвечает по I2C. Обновляется в pulseInit() и pulseCheckConnection().
extern bool pulseSensorConnected;

// Инициализация (в setup)
bool pulseInit();

// Повторная проверка связи с датчиком.
// Если датчик был не найден — пробует переинициализировать.
// Если был найден — проверяет, не отвалился ли.
// Возвращает pulseSensorConnected.
bool pulseCheckConnection();

// Основной цикл обработки (в loop, часто)
void pulseUpdate();

// Получить текущие данные
PulseData pulseGetData();

// Есть ли валидные данные для отображения
bool pulseHasValidData();

const char* pulseStatusToString(PulseStatus status);

#endif
