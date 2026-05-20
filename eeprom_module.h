#ifndef EEPROM_MODULE_H
#define EEPROM_MODULE_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

extern bool eepromFound;
// Структура данных (ровно 32 байта)
struct WeatherData {
  int16_t temperature;       // 2 байта (°C * 10)
  char condition[14];        // 14 байт
  char wind[8];              // 8 байт
  int16_t windDir;           // 2 байта
  uint32_t timestamp;        // 4 байта (unix time)
  uint8_t valid;             // 1 байт
  uint8_t checksum;          // 1 байт
} __attribute__((packed));   // <-- БЕЗ ПАДДИНГА!



// Функции
void eepromInit();
bool eepromWriteSlot(uint8_t slot, uint8_t* data);
bool eepromReadSlot(uint8_t slot, uint8_t* buffer);
bool eepromIsFound();

#endif
