#ifndef RTC_MODULE_H
#define RTC_MODULE_H

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <Timezone.h>

// Часовой пояс (GMT+3 Москва)
extern TimeChangeRule myStandardTime;
extern TimeChangeRule myDaylightSavingsTime;
extern Timezone myTZ;
extern TimeChangeRule *tcr;

// Функции
void rtcInit();
void rtcSyncFromModule();           // Синхронизировать system time с RTC
void rtcSaveToModule();             // Сохранить system time в RTC
bool rtcIsFound();
time_t rtcGetNtpTime();               // Получить время с NTP
void rtcSyncNtpIfNeeded();            // Синхронизация с NTP при необходимости
void syncTimeFromGPS();                // Синхронизация по GPS

// Глобальные переменные
extern RTC_DS1307 rtc;
extern bool rtcFound;
extern bool ntpSynced;
extern time_t lastNtpSync;
extern bool gpsTimeSynced;
extern bool needUpdateTimeScreen;
extern int currentScreen;
extern lastScreen
#endif
