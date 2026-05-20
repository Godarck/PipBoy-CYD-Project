#ifndef PULSE_SENSOR_H
#define PULSE_SENSOR_H

#include <Arduino.h>
#include "config.h"

#ifndef PULSE_SENSOR_PIN
  #define PULSE_SENSOR_PIN    35
#endif

#ifndef BUZZER_PIN
  #define BUZZER_PIN    255
#endif

// Параметры АЦП ESP32 — ИСПРАВЛЕНЫ под твои данные
#define ISP_ADC_RESOLUTION    4096
#define ISP_ADC_LIMIT_MAX     4090    // Было 3600, твой датчик дает ~4000
#define ISP_ADC_LIMIT_MIN     100     // Было 200, твой "нет пальца" ~1800
#define ISP_ADC_FLUCTUATION   80
#define FINGER_RAW_THRESHOLD  1900    // Было 2000, твой "палец" ~4000

// Состояния
#define ISP_DISCONNECTED      0
#define ISP_CONNECTED         1
#define ISP_CHANGED           2

// Типы check()
#define ISP_ANALOG            0
#define ISP_BEEP              1
#define ISP_PULSE             2
#define ISP_VALID             3

class PulseSensor {
public:
    PulseSensor(uint8_t sensorPin = PULSE_SENSOR_PIN, uint8_t beepPin = BUZZER_PIN);
    
    void begin();
    uint16_t check(uint8_t type);
    
    // Проверки состояния
    bool isConnected();      // Датчик подключен (RAW не 0 и не 4095)
    bool onFinger();         // Палец приложен (RAW > порога)
    bool isChanged();        // Состояние изменилось
    
    // Геттеры данных
    int getBPM() { return check(ISP_PULSE); }
    int getRaw() { return check(ISP_ANALOG); }

private:
    uint8_t pinSensor;
    uint8_t pinBeep;
    
    // Буфер 10 значений
    uint16_t dataPIN[10];
    uint16_t dataMAX;
    uint16_t dataMIN;
    uint16_t dataCEN;
    
    // Состояние
    bool flagVAL;
    bool flagTOP;
    uint8_t dataTOP;
    uint16_t timeTOP;
    uint16_t timeCNT;
    
    // Время пульсов
    uint16_t timeARR[5];
    unsigned long timeNOW;
    unsigned long timeWAS;
    
    // Биппер
    bool flagBEP;
    bool dataBEP;
    
    // Тайминг
    uint32_t lastSample;
    uint32_t lastProcess;  // Когда последний раз обрабатывали
    
    // Методы
    uint16_t checkBeep();
    uint16_t checkPulse();
    uint8_t checkValid();
    void processSample();
};

extern PulseSensor pulse;

#endif
