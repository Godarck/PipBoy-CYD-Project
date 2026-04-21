#ifndef WEATHER_MODULE_H
#define WEATHER_MODULE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>



// Структура текущей погоды
struct CurrentWeather {
  String temperature;
  String condition;
  String wind;
  int windDir;
  unsigned long lastUpdate;    // millis() когда получено
  uint8_t source;              // 0=wttr.in, 1=open-meteo, 2=eeprom
};

// Функции
void weatherInit();
void weatherUpdate();            // Основная функция обновления
void weatherForceUpdate();     // Принудительное обновление
bool weatherLoadFromEEPROM();  // Загрузка из EEPROM
void weatherSaveToEEPROM();    // Сохранение в EEPROM
int arrowToDegrees();
time_t weatherLastUpdate();

// Геттеры
CurrentWeather* weatherGetCurrent();
int weatherGetAgeMinutes();    // Возраст данных в минутах
bool weatherHasData();

// Глобальные настройки (изменяются через UI)
extern String weatherLat;
extern String weatherLon;
extern bool weatherCelsius;

#endif
