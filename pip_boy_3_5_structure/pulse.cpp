#include "pulse.h"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "config.h"

// --- Настройки графика ---
#define GRAPH_WIDTH   60
#define GRAPH_HEIGHT  25
#define GRAPH_MIN_RANGE 500   // Минимальный размах, чтобы шум не скакал

extern TFT_eSPI tft;

bool pulseSensorConnected = false;

static MAX30105 particleSensor;
static uint32_t irBuffer[BUFFER_LENGTH];
static uint32_t redBuffer[BUFFER_LENGTH];
static int32_t spo2Value;
static int8_t validSPO2;
static int32_t heartRate;
static int8_t validHeartRate;

static PulseData currentData = {0, 0, 0.0f, false, PULSE_CALIBRATING};
static bool sensorInitialized = false;
static uint16_t bufferIndex = 0;
static unsigned long calibrationStart = 0;
static unsigned long lastTempRead = 0;

static int bpmHistory[5] = {0};
static uint8_t bpmHistoryIndex = 0;

// --- Буфер графика ---
static uint32_t irGraphBuffer[GRAPH_WIDTH];
static uint8_t graphWriteIndex = 0;
static bool graphFilled = false;

// --- Спрайт графика (создаётся один раз) ---
static TFT_eSprite graphSprite(&tft);

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
        Serial.println("[PULSE] MAX30102 не найден!");
        currentData.status = PULSE_SENSOR_NOT_FOUND;
        sensorInitialized = false;
        return false;
    }

    Serial.println("[PULSE] MAX30102 обнаружен");
    applySensorSettings();

    sensorInitialized = true;
    currentData.status = PULSE_CALIBRATING;
    bufferIndex = 0;
    calibrationStart = millis();
    lastTempRead = millis();

    // Инициализация буфера графика
    graphWriteIndex = 0;
    graphFilled = false;
    for (int i = 0; i < GRAPH_WIDTH; i++) irGraphBuffer[i] = 0;

    // Создание спрайта (один раз)
    graphSprite.createSprite(GRAPH_WIDTH, GRAPH_HEIGHT);
    graphSprite.fillSprite(TFT_BLACK);

    for (int i = 0; i < 5; i++) bpmHistory[i] = 0;
    
    return true;
}

bool pulseCheckConnection() {
    if (!pulseSensorConnected) {
        pulseSensorConnected = particleSensor.begin(Wire, I2C_SPEED_FAST);
        if (pulseSensorConnected) {
            Serial.println("[PULSE] Датчик найден");
            applySensorSettings();
            sensorInitialized = true;
            currentData.status = PULSE_CALIBRATING;
            bufferIndex = 0;
            calibrationStart = millis();
            lastTempRead = millis();
        }
    } else {
        Wire.beginTransmission(MAX30102_I2C_ADDR);
        uint8_t error = Wire.endTransmission();
        if (error != 0) {
            Serial.printf("[PULSE] Датчик потерян (err:%d)\n", error);
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
            Serial.println("[PULSE] Таймаут калибровки, сброс");
            bufferIndex = 0;
            calibrationStart = millis();
        }
    }

    // Температура
    if (millis() - lastTempRead > TEMP_READ_INTERVAL_MS) {
        lastTempRead = millis();
        currentData.temperature = particleSensor.readTemperature();
    }

    // --- Основной цикл чтения ---
    while (particleSensor.check()) {
        uint32_t irValue = particleSensor.getIR();
        uint32_t redValue = particleSensor.getRed();

        // --- Сохраняем в буфер графика (каждый сэмпл) ---
        irGraphBuffer[graphWriteIndex] = irValue;
        graphWriteIndex++;
        if (graphWriteIndex >= GRAPH_WIDTH) {
            graphWriteIndex = 0;
            graphFilled = true;
        }

        if (irValue >= 262000 || redValue >= 262000) {
            particleSensor.nextSample();

            //if (DEBUGFLAG) Serial.printf("[pulse] OUT OF 262000 - irValue:%d  redValue:%d\n", irValue, redValue);
            continue;
        }

        // Двойная проверка пальца: IR + RED + соотношение
        if (irValue < PULSE_FINGER_IR_THRESHOLD || 
            redValue < PULSE_FINGER_RED_MIN ) {
            //|| (float)irValue / redValue < 1.2f) {
            
            currentData.fingerDetected = false;
            currentData.status = PULSE_FINGER_NOT_DETECTED;
            bufferIndex = 0;
            calibrationStart = millis();
            particleSensor.nextSample();

            //if (DEBUGFLAG) Serial.printf("[pulse] OUT OF PULSE_FINGER_IR_THRESHOLD - irValue:%d  redValue:%d diff: %.2f \n", irValue, redValue, (float)(irValue / redValue));
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
                for (int i = 0; i < 25; i++) {
                    redBuffer[i] = redBuffer[BUFFER_LENGTH - 25 + i];
                    irBuffer[i] = irBuffer[BUFFER_LENGTH - 25 + i];
                }
                bufferIndex = 25;
                continue;
            }

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

// --- ОТРИСОВКА ГРАФИКА ---
void pulseDrawGraph(int x, int y, uint16_t color) {
    if (!sensorInitialized) return;

    graphSprite.fillSprite(TFT_BLACK);

    uint8_t count = graphFilled ? GRAPH_WIDTH : graphWriteIndex;
    if (count < 2) return;

    // Находим min/max для авто-масштаба
    uint32_t minVal = irGraphBuffer[0];
    uint32_t maxVal = irGraphBuffer[0];
    for (uint8_t i = 1; i < count; i++) {
        if (irGraphBuffer[i] < minVal) minVal = irGraphBuffer[i];
        if (irGraphBuffer[i] > maxVal) maxVal = irGraphBuffer[i];
    }

    uint32_t range = maxVal - minVal;
    if (range < GRAPH_MIN_RANGE) range = GRAPH_MIN_RANGE;

    // Временный массив: temp[0] = самая старая (слева), temp[59] = самая новая (справа)
    uint32_t temp[GRAPH_WIDTH];
    if (graphFilled) {
        for (int i = 0; i < GRAPH_WIDTH; i++) {
            int idx = (graphWriteIndex + i) % GRAPH_WIDTH;
            temp[i] = irGraphBuffer[idx];
        }
    } else {
        for (int i = 0; i < count; i++) temp[i] = irGraphBuffer[i];
        for (int i = count; i < GRAPH_WIDTH; i++) temp[i] = irGraphBuffer[count > 0 ? count - 1 : 0];
    }

    // Рисуем линию
    for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
        int y1 = GRAPH_HEIGHT - 1 - (int)((temp[i]   - minVal) * (GRAPH_HEIGHT - 1) / range);
        int y2 = GRAPH_HEIGHT - 1 - (int)((temp[i+1] - minVal) * (GRAPH_HEIGHT - 1) / range);

        y1 = constrain(y1, 0, GRAPH_HEIGHT - 1);
        y2 = constrain(y2, 0, GRAPH_HEIGHT - 1);

        graphSprite.drawLine(i, y1, i + 1, y2, color);
    }

    graphSprite.pushSprite(x, y);
}
