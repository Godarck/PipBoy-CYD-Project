#include "pulse_sensor.h"

PulseSensor pulse(PULSE_SENSOR_PIN, BUZZER_PIN);

PulseSensor::PulseSensor(uint8_t sensorPin, uint8_t beepPin) {
    pinSensor = sensorPin;
    pinBeep = beepPin;
}

void PulseSensor::begin() {
    if (pinSensor >= 255)
    return;
    pinMode(pinSensor, INPUT);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    
    flagVAL = false;
    dataMAX = 0;
    dataMIN = ISP_ADC_RESOLUTION;
    timeCNT = 0;
    dataTOP = 0;
    flagTOP = false;
    timeTOP = 0;
    
    for (uint8_t i = 0; i < 5; i++) timeARR[i] = 0;
    for (uint8_t i = 0; i < 10; i++) dataPIN[i] = 0;
    
    timeNOW = 0;
    timeWAS = 0;
    
    flagBEP = (pinBeep < 255);
    dataBEP = false;
    if (flagBEP) {
        pinMode(pinBeep, OUTPUT);
        digitalWrite(pinBeep, LOW);
    }
    
    lastSample = 0;
    lastProcess = 0;
}

uint16_t PulseSensor::check(uint8_t type) {
    // ИСПРАВЛЕНО: обрабатываем ВСЕ накопленные выборки сразу
    // Не важно, вызывается ли раз в секунду или чаще
    uint32_t now = millis();
    
    // Сколько выборок пропустили?
    uint32_t elapsed = now - lastProcess;
    uint32_t samplesNeeded = elapsed / 4;  // 4мс на выборку
    
    // Ограничиваем чтобы не зависнуть
    if (samplesNeeded > 500) samplesNeeded = 500;
    
    for (uint32_t i = 0; i < samplesNeeded; i++) {
        processSample();
    }
    
    if (samplesNeeded > 0) {
        lastProcess = now;
    }
    
    switch(type) {
        case ISP_ANALOG: return dataPIN[0];
        case ISP_BEEP: return checkBeep();
        case ISP_PULSE: return checkPulse();
        case ISP_VALID: return checkValid();
    }
    return 0;
}

// ======================= НОВЫЕ МЕТОДЫ =======================

bool PulseSensor::isConnected() {
    // Датчик подключен если RAW не в saturation
    int raw = getRaw();
    return (raw > 50) && (raw < 4080);
}

bool PulseSensor::onFinger() {
    // ИСПРАВЛЕНО: просто по RAW, без сложной валидации
    int raw = getRaw();
    return raw > FINGER_RAW_THRESHOLD;
}

bool PulseSensor::isChanged() {
    return check(ISP_VALID) == ISP_CHANGED;
}

// ======================= ОБРАБОТКА =======================

void PulseSensor::processSample() {
    if (pinSensor >= 255)
    return;
    timeCNT++;
    
    if (timeCNT % 4 == 0) {
        if (timeTOP < 0xFFFF) timeTOP++;
        dataTOP = 0;
        
        // Сдвиг буфера
        for (uint8_t i = 9; i > 0; i--) {
            dataPIN[i] = dataPIN[i-1];
            if (dataPIN[i] > dataCEN) dataTOP++;
        }
        
        dataPIN[0] = analogRead(pinSensor);
        
        // Обновление MIN/MAX за 2 секунды (500 выборок по 4мс)
        if (timeCNT >= 500) {
            timeCNT = 0;
            dataCEN = dataMIN + (dataMAX - dataMIN) * 2 / 3;
            dataMAX = 0;
            dataMIN = ISP_ADC_RESOLUTION;
        }
        
        if (dataMAX < dataPIN[0]) dataMAX = dataPIN[0];
        if (dataMIN > dataPIN[0]) dataMIN = dataPIN[0];
        
        // ДЕТЕКЦИЯ ВЕРШИНЫ
        if (dataTOP > 7) {
            if (!flagTOP) {
                for (uint8_t i = 4; i > 0; i--) {
                    timeARR[i] = timeARR[i-1];
                }
                
                timeWAS = timeNOW;
                timeNOW = millis();
                if (timeWAS >= timeNOW) timeWAS = 0;
                
                timeARR[0] = timeNOW - timeWAS;
                if (timeARR[0] > 0) {
                    timeARR[0] = 60000 / timeARR[0];
                }
                
                timeTOP = 0;
            }
            flagTOP = true;
        }
        
        if (dataTOP < 3) flagTOP = false;
        
        // Биппер
        if (flagBEP && flagVAL && timeTOP > 0 && timeTOP < 25) {
            dataBEP = !dataBEP;
            //digitalWrite(pinBeep, dataBEP);
						tone(pinBeep, 600, 8);
        }
    }
}

uint16_t PulseSensor::checkBeep() {
    return timeTOP / 25;
}

uint16_t PulseSensor::checkPulse() {
    for (uint8_t i = 0; i < 5; i++) {
        if (timeARR[i] < 10) return 0;
    }
    
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 5; i++) sum += timeARR[i];
    uint16_t result = sum / 5;
    
    if (result > 999) result = 999;
    return result;
}

uint8_t PulseSensor::checkValid() {
    uint8_t i1, i2;
    uint8_t j1 = 0, j2 = 0;
    uint8_t prevVAL = flagVAL;
    
    // Проверка флуктуаций
    if (dataPIN[0] > ISP_ADC_LIMIT_MAX || dataPIN[0] < ISP_ADC_LIMIT_MIN) {
        // Значение вне диапазона — возможно нет пальца или обрыв
    }
    
    if (dataPIN[0] - ISP_ADC_FLUCTUATION > dataPIN[1]) i1 = 1; else i1 = 0;
    
    for (uint8_t k = 1; k < 9; k++) {
        if (dataPIN[k] - ISP_ADC_FLUCTUATION > dataPIN[k+1]) i2 = 1; else i2 = 0;
        if (dataPIN[k] + ISP_ADC_FLUCTUATION < dataPIN[k+1]) i2 = 0;
        
        if (i1 != i2) j1++;
        if (i2) i1 = 1; else i1 = 0;
        
        if (dataPIN[k] > ISP_ADC_LIMIT_MAX || dataPIN[k] < ISP_ADC_LIMIT_MIN) {
            j2++;
        }
    }
    
    // ИСПРАВЛЕНО: валидация мягче
    flagVAL = true;
    if (dataPIN[0] < FINGER_RAW_THRESHOLD) flagVAL = false;  // Нет пальца
    if (j1 > 5) flagVAL = false;                               // Шум (было 3)
    if (j2 > 8) flagVAL = false;                               // Вне диапазона (было 5)
    if (checkPulse() > 300) flagVAL = false;
    
    if (prevVAL != flagVAL) return ISP_CHANGED;
    if (flagVAL) return ISP_CONNECTED;
    return ISP_DISCONNECTED;
}
