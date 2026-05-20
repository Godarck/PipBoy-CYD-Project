// GPSModule.h
#pragma once
#include <Arduino.h>

class GPSModule {
public:
    GPSModule(HardwareSerial& serial, uint8_t rxPin, uint8_t txPin, uint32_t baud = 9600);
    
    bool begin();
    bool init(uint16_t timeoutMs = 2000, uint8_t retries = 3);
    void update();

    bool     hasFix()       const { return _fix; }        // Есть координаты
    bool     hasTime()      const { return _timeValid; }   // Есть время (любой фикс)
    double   getLat()       const { return _lat; }
    double   getLng()       const { return _lng; }
    double   getAlt()       const { return _alt; }
    uint8_t  getSats()      const { return _sats; }
    double   getSpeedKmph() const { return _speed; }
    uint8_t  getFixQuality() const { return _fixQuality; }

    void getTime(uint8_t& h, uint8_t& m, uint8_t& s) const;
    void getDate(uint8_t& d, uint8_t& mo, uint8_t& y) const;
    String getTimeString() const;
    String getDateString() const;
    String getCoordString() const;  // "Lat: xx.xxxx, Lng: yy.yyyy"

private:
    HardwareSerial* _serial;
    uint8_t  _rx, _tx;
    uint32_t _baud;

    bool     _fix, _timeValid;
    double   _lat, _lng, _alt, _speed;
    uint8_t  _sats, _fixQuality;
    uint8_t  _hour, _minute, _second;
    uint8_t  _day, _month, _year;

    char     _buf[128];
    uint8_t  _len;
    uint32_t _lastNmeaTime;

    void processLine();
    void parseGGA(char* line);
    void parseRMC(char* line);
    static double parseCoord(const char* s, char dir);
    static bool   verifyChecksum(const char* line);
    
    void sendUbxPoll(uint8_t cls, uint8_t id);
    bool waitForUbx(uint8_t cls, uint8_t id, uint16_t timeoutMs);
};
