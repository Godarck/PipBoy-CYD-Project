#ifndef PULSE_H
#define PULSE_H

#include <Arduino.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

enum PulseStatus {
    PULSE_OK = 0,
    PULSE_SENSOR_NOT_FOUND,
    PULSE_FINGER_NOT_DETECTED,
    PULSE_CALIBRATING
};

struct PulseData {
    int bpm;
    int spo2;
    float temperature;
    bool fingerDetected;
    PulseStatus status;
};

extern bool pulseSensorConnected;

bool pulseInit();
bool pulseCheckConnection();
void pulseUpdate();
PulseData pulseGetData();
bool pulseHasValidData();
const char* pulseStatusToString(PulseStatus status);

// --- НОВОЕ: отрисовка графика пульсаций ---
// x,y — координаты левого верхнего угла на экране
void pulseDrawGraph(int x, int y, uint16_t color = TFT_GREEN);

#endif
