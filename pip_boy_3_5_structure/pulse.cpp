#include "pulse.h"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"



bool pulseSensorConnected = false;

static MAX30105 particleSensor;
static uint32_t irBuffer[BUFFER_LENGTH];
static uint32_t redBuffer[BUFFER_LENGTH];
static int32_t spo2Value;
static int8_t validSPO2;
static int32_t heartRate;
static int8_t validHeartRate;

static PulseData currentData = {0, 0, false, PULSE_CALIBRATING};
static bool sensorInitialized = false;
static uint16_t bufferIndex = 0;
static unsigned long calibrationStart = 0;

static int bpmHistory[5] = {0};
static uint8_t bpmHistoryIndex = 0;

static int smoothBPM(int rawBPM) {
    bpmHistory[bpmHistoryIndex] = rawBPM;
    bpmHistoryIndex = (bpmHistoryIndex + 1) % 5;
    int sum = 0, count = 0;
    for (int i = 0; i < 5; i++) {
        if (bpmHistory[i] > 30 && bpmHistory[i] < 250) {
            sum += bpmHistory[i];
            count++;
        }
    }
    return (count > 0) ? (sum / count) : 0;
}

static void applySensorSettings() {
    // Как в примере: setup(brightness, average, mode, rate, width, range)
    particleSensor.setup(PULSE_LED_BRIGHTNESS, PULSE_SAMPLE_AVERAGE, PULSE_LED_MODE, 
                         PULSE_SAMPLE_RATE, PULSE_PULSE_WIDTH, PULSE_ADC_RANGE);
    particleSensor.setPulseAmplitudeRed(0x1A);
    particleSensor.setPulseAmplitudeIR(0x1A);
    particleSensor.enableDIETEMPRDY();
}

bool pulseInit() {
    Wire.begin(PULSE_I2C_SDA, PULSE_I2C_SCL);
    
    pulseSensorConnected = particleSensor.begin(Wire, I2C_SPEED_FAST);
    
    if (!pulseSensorConnected) {
        if (DEBUGFLAG) Serial.println("[PULSE] MAX30102 не найден!");
        currentData.status = PULSE_SENSOR_NOT_FOUND;
        sensorInitialized = false;
        return false;
    }

    if (DEBUGFLAG) Serial.println("[PULSE] MAX30102 обнаружен");
    applySensorSettings();

    sensorInitialized = true;
    currentData.status = PULSE_CALIBRATING;
    bufferIndex = 0;
    calibrationStart = millis();
    for (int i = 0; i < 5; i++) bpmHistory[i] = 0;
    
    return true;
}

bool pulseCheckConnection() {
    if (!pulseSensorConnected) {
        pulseSensorConnected = particleSensor.begin(Wire, I2C_SPEED_FAST);
        if (pulseSensorConnected) {
            if (DEBUGFLAG) Serial.println("[PULSE] Датчик найден");
            applySensorSettings();
            sensorInitialized = true;
            currentData.status = PULSE_CALIBRATING;
            bufferIndex = 0;
            calibrationStart = millis();
        }
    } else {
        Wire.beginTransmission(MAX30102_I2C_ADDR);
        uint8_t error = Wire.endTransmission();
        if (error != 0) {
            if (DEBUGFLAG) Serial.printf("[PULSE] Датчик потерян (err:%d)\n", error);
            pulseSensorConnected = false;
            sensorInitialized = false;
            currentData.status = PULSE_SENSOR_NOT_FOUND;
        }
    }
    return pulseSensorConnected;
}

void pulseUpdate() {
    if (!sensorInitialized || !pulseSensorConnected) return;

    if (currentData.status == PULSE_CALIBRATING) {
        if (millis() - calibrationStart > CALIBRATION_TIMEOUT_MS) {
            if (DEBUGFLAG) Serial.println("[PULSE] Таймаут калибровки, сброс");
            bufferIndex = 0;
            calibrationStart = millis();
        }
    }

    // --- КАК В ПРИМЕРЕ: while (particleSensor.check()) ---
    // check() читает один сэмпл из FIFO чипа во внутренний буфер библиотеки
    // и возвращает true, если сэмпл появился
    while (particleSensor.check()) {
        uint32_t irValue = particleSensor.getIR();
        uint32_t redValue = particleSensor.getRed();

        // Зашкалившие сэмплы — пропускаем
        if (irValue >= 262000 || redValue >= 262000) {
            particleSensor.nextSample();
            continue;
        }

        // Нет пальца
        if (irValue < PULSE_FINGER_IR_THRESHOLD) {
            currentData.fingerDetected = false;
            currentData.status = PULSE_FINGER_NOT_DETECTED;
            bufferIndex = 0;
            calibrationStart = millis();
            particleSensor.nextSample();
            continue;
        }

        currentData.fingerDetected = true;

        redBuffer[bufferIndex] = redValue;
        irBuffer[bufferIndex] = irValue;
        bufferIndex++;
        particleSensor.nextSample();

        if (bufferIndex >= BUFFER_LENGTH) {
            maxim_heart_rate_and_oxygen_saturation(
                irBuffer, BUFFER_LENGTH, 
                redBuffer, 
                &spo2Value, &validSPO2, 
                &heartRate, &validHeartRate
            );

            if (validHeartRate && heartRate > 30 && heartRate < 250) {
                currentData.bpm = smoothBPM((int)heartRate);
            }
            if (validSPO2 && spo2Value > 70 && spo2Value <= 100) {
                currentData.spo2 = (int)spo2Value;
            }

            if (validHeartRate) {
                currentData.status = PULSE_OK;
            } else {
                currentData.status = PULSE_CALIBRATING;
                // Оставляем 25 сэмплов для overlap
                for (int i = 0; i < 25; i++) {
                    redBuffer[i] = redBuffer[BUFFER_LENGTH - 25 + i];
                    irBuffer[i] = irBuffer[BUFFER_LENGTH - 25 + i];
                }
                bufferIndex = 25;
                continue;
            }

            // Сдвиг для следующего цикла
            for (int i = 0; i < 25; i++) {
                redBuffer[i] = redBuffer[BUFFER_LENGTH - 25 + i];
                irBuffer[i] = irBuffer[BUFFER_LENGTH - 25 + i];
            }
            bufferIndex = 25;
        }
    }
}

PulseData pulseGetData() {
    return currentData;
}

bool pulseHasValidData() {
    return sensorInitialized && 
           pulseSensorConnected &&
           currentData.status == PULSE_OK && 
           currentData.fingerDetected &&
           currentData.bpm > 0;
}

const char* pulseStatusToString(PulseStatus status) {
    switch (status) {
        case PULSE_OK:                return "OK";
        case PULSE_SENSOR_NOT_FOUND:  return "NOT_FOUND";
        case PULSE_FINGER_NOT_DETECTED: return "NO_FINGER";
        case PULSE_CALIBRATING:       return "CALIBRATING";
        default:                      return "UNKNOWN";
    }
}
