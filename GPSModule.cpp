#include "GPSModule.h"
GPSModule::GPSModule(HardwareSerial& serial, uint8_t rxPin, uint8_t txPin, uint32_t baud)
    : _serial(&serial), _rx(rxPin), _tx(txPin), _baud(baud),
      _fix(false), _timeValid(false), _lat(0), _lng(0), _alt(0), _speed(0),
      _sats(0), _fixQuality(0), _hour(0), _minute(0), _second(0),
      _day(0), _month(0), _year(0), _len(0), _lastNmeaTime(0) {}

bool GPSModule::begin() {
    _serial->begin(_baud, SERIAL_8N1, _rx, _tx);
    _len = 0;
    return true;
}

// Отправка UBX-команды опроса (без payload)
void GPSModule::sendUbxPoll(uint8_t cls, uint8_t id) {
    uint8_t msg[8] = {
        0xB5, 0x62,       // sync
        cls, id,          // class, id
        0x00, 0x00        // payload length = 0
    };
    uint8_t ckA = 0, ckB = 0;
    for (uint8_t i = 2; i < 6; i++) {
        ckA += msg[i];
        ckB += ckA;
    }
    msg[6] = ckA;
    msg[7] = ckB;
    _serial->write(msg, 8);
    _serial->flush();
}

// Ожидание UBX-ответа с заданным class/id
bool GPSModule::waitForUbx(uint8_t cls, uint8_t id, uint16_t timeoutMs) {
    uint32_t start = millis();
    uint8_t state = 0; // 0=wait sync1, 1=wait sync2, 2=class, 3=id, 4=lenL, 5=lenH, 6=payload+ck
    uint8_t rxClass, rxId;
    uint16_t payloadLen;
    uint16_t rxCount;
    uint8_t ckA = 0, ckB = 0;
    uint8_t rxBuf[64]; // буфер для payload

    while (millis() - start < timeoutMs) {
        while (_serial->available()) {
            uint8_t c = _serial->read();

            switch (state) {
                case 0:
                    if (c == 0xB5) state = 1;
                    break;
                case 1:
                    if (c == 0x62) {
                        state = 2;
                        ckA = 0; ckB = 0;
                    } else if (c != 0xB5) {
                        state = 0;
                    }
                    break;
                case 2:
                    rxClass = c;
                    ckA += c; ckB += ckA;
                    state = 3;
                    break;
                case 3:
                    rxId = c;
                    ckA += c; ckB += ckA;
                    state = 4;
                    break;
                case 4:
                    payloadLen = c;
                    ckA += c; ckB += ckA;
                    state = 5;
5;
                    break;
                case 5:
                    payloadLen |= ((uint16_t)c << 8);
                    ckA += c; ckB += ckA;
                    rxCount = 0;
                    state = (payloadLen == 0) ? 7 : 6;
                    break;
                case 6:
                    if (rxCount < sizeof(rxBuf)) rxBuf[rxCount] = c;
                    ckA += c; ckB += ckA;
                    rxCount++;
                    if (rxCount >= payloadLen) state = 7;
                    break;
                case 7:
                    if (c == ckA) {
                        state = 8;
                    } else {
                        state = 0;
                    }
                    break;
                case 8:
                    if (c == ckB) {
                        // Полный валидный пакет получен
                        if (rxClass == cls && rxId == id) {
                            return true;
                        }
                        // Это другой UBX-пакет, продолжаем слушать
                    }
                    state = 0;
                    break;
            }
        }
        yield();
    }
    return false;
}


bool GPSModule::init(uint16_t timeoutMs, uint8_t retries) {
    // Сначала пробуем UBX
    while (_serial->available()) _serial->read();
    
    for (uint8_t i = 0; i < retries; i++) {
        sendUbxPoll(0x0A, 0x04);  // MON-VER
        if (waitForUbx(0x0A, 0x04, timeoutMs / retries)) {
            Serial.println("[GPS] UBX ответ получен");
            return true;
        }
        if (i < retries - 1) delay(100);
    }
    
    // Fallback: проверяем, идут ли NMEA строки
    Serial.println("[GPS] UBX молчит, проверяем NMEA...");
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        while (_serial->available()) {
            char c = _serial->read();
            if (c == '$') {
                // Дождались начала NMEA — модуль жив
                Serial.println("[GPS] NMEA поток обнаружен, модуль доступен");
                return true;
            }
        }
        yield();
    }
    
    Serial.println("[GPS] Нет ни UBX, ни NMEA — модуль не отвечает");
    return false;
}


void GPSModule::update() {
    while (_serial->available()) {
        char c = _serial->read();
        if (c == '$') {
            _len = 0;
            _buf[_len++] = c;
        } else if (_len > 0 && _len < sizeof(_buf) - 1) {
            _buf[_len++] = c;
            if (c == '\n' || c == '\r') {
                _buf[_len] = '\0';
                _lastNmeaTime = millis();
                processLine();
                _len = 0;
            }
        } else {
            _len = 0;
        }
    }
}

void GPSModule::parseGGA(char* line) {
    char* save = nullptr;
    char* tok = strtok_r(line, ",", &save); // $xxGGA

    tok = strtok_r(nullptr, ",", &save); // время
    if (tok && strlen(tok) >= 6) {
        _hour   = (tok[0] - '0') * 10 + (tok[1] - '0');
        _minute = (tok[2] - '0') * 10 + (tok[3] - '0');
        _second = (tok[4] - '0') * 10 + (tok[5] - '0');
        _timeValid = true;  // ← Время есть!
    }

    tok = strtok_r(nullptr, ",", &save); // широта
    char* latStr = tok;
    tok = strtok_r(nullptr, ",", &save); // N/S
    char latDir = tok ? tok[0] : 'N';

    tok = strtok_r(nullptr, ",", &save); // долгота
    char* lonStr = tok;
    tok = strtok_r(nullptr, ",", &save); // E/W
    char lonDir = tok ? tok[0] : 'E';

    tok = strtok_r(nullptr, ",", &save); // качество фикса
    _fixQuality = tok ? atoi(tok) : 0;
    _fix = (_fixQuality > 0);  // 1=GPS, 2=DGPS — есть координаты

    tok = strtok_r(nullptr, ",", &save); // спутники
    _sats = tok ? atoi(tok) : 0;

    tok = strtok_r(nullptr, ",", &save); // HDOP
    tok = strtok_r(nullptr, ",", &save); // высота
    _alt = tok ? atof(tok) : 0.0;

    _lat = parseCoord(latStr, latDir);
    _lng = parseCoord(lonStr, lonDir);
}

void GPSModule::parseRMC(char* line) {
    char* save = nullptr;
    char* tok = strtok_r(line, ",", &save); // $xxRMC

    tok = strtok_r(nullptr, ",", &save); // время
    if (tok && strlen(tok) >= 6) {
        _hour   = (tok[0] - '0') * 10 + (tok[1] - '0');
        _minute = (tok[2] - '0') * 10 + (tok[3] - '0');
        _second = (tok[4] - '0') * 10 + (tok[5] - '0');
        _timeValid = true;
    }

    tok = strtok_r(nullptr, ",", &save); // статус A/V
    char status = tok ? tok[0] : 'V';
    if (status == 'A') _fix = true;  // RMC говорит "Active" — тоже фикс

    tok = strtok_r(nullptr, ",", &save); // широта
    char* latStr = tok;
    tok = strtok_r(nullptr, ",", &save); // N/S
    char latDir = tok ? tok[0] : 'N';

    tok = strtok_r(nullptr, ",", &save); // долгота
    char* lonStr = tok;
    tok = strtok_r(nullptr, ",", &save); // E/W
    char lonDir = tok ? tok[0] : 'E';

    tok = strtok_r(nullptr, ",", &save); // скорость в узлах
    double knots = tok ? atof(tok) : 0.0;
    _speed = knots * 1.852;

    tok = strtok_r(nullptr, ",", &save); // курс
    tok = strtok_r(nullptr, ",", &save); // дата
    if (tok && strlen(tok) == 6) {
        _day   = (tok[0] - '0') * 10 + (tok[1] - '0');
        _month = (tok[2] - '0') * 10 + (tok[3] - '0');
        _year  = (tok[4] - '0') * 10 + (tok[5] - '0');
    }

    _lat = parseCoord(latStr, latDir);
    _lng = parseCoord(lonStr, lonDir);
}

String GPSModule::getCoordString() const {
    char b[64];
    snprintf(b, sizeof(b), "Lat: %.6f, Lng: %.6f", _lat, _lng);
    return String(b);
}


bool GPSModule::verifyChecksum(const char* line) {
    if (line[0] != '$') return false;
    const char* star = strchr(line, '*');
    if (!star || strlen(star) < 3) return false;
    uint8_t sum = 0;
    for (const char* p = line + 1; p < star; ++p) sum ^= *p;
    uint8_t chk = (uint8_t)strtol(star + 1, nullptr, 16);
    return sum == chk;
}

double GPSModule::parseCoord(const char* s, char dir) {
    if (!s || *s == '\0') return 0.0;
    double raw = atof(s);
    double deg = (int)(raw / 100.0);
    double min = raw - deg * 100.0;
    double dec = deg + min / 60.0;
    if (dir == 'S' || dir == 'W') dec = -dec;
    return dec;
}

void GPSModule::processLine() {
    if (!verifyChecksum(_buf)) return;

    if (strncmp(_buf, "$GPGGA", 6) == 0 || strncmp(_buf, "$GNGGA", 6) == 0) {
        parseGGA(_buf);
    } else if (strncmp(_buf, "$GPRMC", 6) == 0 || strncmp(_buf, "$GNRMC", 6) == 0) {
        parseRMC(_buf);
    }
}

void GPSModule::getTime(uint8_t& h, uint8_t& m, uint8_t& s) const {
    h = _hour; m = _minute; s = _second;
}

void GPSModule::getDate(uint8_t& d, uint8_t& mo, uint8_t& y) const {
    d = _day; mo = _month; y = _year;
}

String GPSModule::getTimeString() const {
    char b[10];
    snprintf(b, sizeof(b), "%02d:%02d:%02d", _hour, _minute, _second);
    return String(b);
}

String GPSModule::getDateString() const {
    char b[10];
    snprintf(b, sizeof(b), "%02d.%02d.%02d", _day, _month, _year);
    return String(b);
}
