#include "gps_interface.h"
#include "config.h"

// ============================================
// РЕЖИМ: I2C + ESP32-C3 (умный мост)
// ============================================
#ifdef GPS_USE_I2C_C3

#include <Wire.h>

struct __attribute__((packed)) GpsPacket {
  uint8_t  magic;
  uint8_t  fix;         // fixMode: 0=нет, 1=2D, 2=3D
  uint8_t  sats;
  int32_t  lat;         // microdegrees
  int32_t  lon;         // microdegrees
  uint16_t year;
  uint8_t  month, day, hour, minute, second;
  uint16_t hdop;        // *100
  uint8_t  led_mode;
  uint8_t  led_r, led_g, led_b;
  uint8_t  fixQuality;  // TinyGPS++ 1.1.0: 0=invalid, 1=GPS, 2=DGPS, 4=RTK fixed...  	
  uint16_t speed;       // сотые км/ч (1234 = 12.34 km/h)
  int32_t  altitude;    // сантиметры (12345 = 123.45 m)
  uint16_t course;      // сотые градуса (9000 = 90.00°)
  uint8_t  checksum;
};

static GpsPacket lastPacket;
static bool      packetValid = false;
static unsigned long lastReadMs = 0;

static inline float udegToDeg(int32_t v) { return v / 1000000.0f; }

void gpsInit() {
  Wire.begin();
}

bool gpsCheckConnection() {
  for (int attempt = 0; attempt < 3; attempt++) {
    uint8_t buf[GPS_I2C_PACKET_SIZE];
    int got = Wire.requestFrom((uint8_t)C3_I2C_ADDR, (uint8_t)GPS_I2C_PACKET_SIZE);
    if (got != GPS_I2C_PACKET_SIZE) { delay(50); continue; }

    int i = 0;
    while (Wire.available() && i < GPS_I2C_PACKET_SIZE) buf[i++] = Wire.read();
    if (i != GPS_I2C_PACKET_SIZE) { delay(50); continue; }
    if (buf[0] != GPS_PACKET_MAGIC) { delay(50); continue; }

    uint8_t cs = 0;
    for (int j = 0; j < GPS_I2C_PACKET_SIZE - 1; j++) cs ^= buf[j];
    if (cs == buf[GPS_I2C_PACKET_SIZE - 1]) {
      memcpy(&lastPacket, buf, sizeof(lastPacket));
      packetValid = true;
      return true;
    }
    delay(50);
  }
  return false;
}

void gpsUpdate() {
  if (millis() - lastReadMs < 200) return;
  lastReadMs = millis();

  uint8_t buf[GPS_I2C_PACKET_SIZE];
  int got = Wire.requestFrom((uint8_t)C3_I2C_ADDR, (uint8_t)GPS_I2C_PACKET_SIZE);
  if (got != GPS_I2C_PACKET_SIZE) { packetValid = false; return; }

  int i = 0;
  while (Wire.available() && i < GPS_I2C_PACKET_SIZE) buf[i++] = Wire.read();
  if (buf[0] != GPS_PACKET_MAGIC) { packetValid = false; return; }

  uint8_t cs = 0;
  for (int j = 0; j < GPS_I2C_PACKET_SIZE - 1; j++) cs ^= buf[j];
  if (cs != buf[GPS_I2C_PACKET_SIZE - 1]) { packetValid = false; return; }

  memcpy(&lastPacket, buf, sizeof(lastPacket));
  packetValid = true;
}

bool gpsHasFix()     { return packetValid && lastPacket.fix >= 2; }
uint8_t gpsGetSats() { return packetValid ? lastPacket.sats : 0; }
float gpsGetLat()    { return packetValid ? udegToDeg(lastPacket.lat) : 0.0f; }
float gpsGetLon()    { return packetValid ? udegToDeg(lastPacket.lon) : 0.0f; }
float gpsGetHdop()   { return packetValid ? (lastPacket.hdop / 100.0f) : 99.99f; }

uint8_t gpsGetFixQuality() { return packetValid ? lastPacket.fixQuality : 0; }
uint8_t gpsGetFixMode()    { return packetValid ? lastPacket.fix : 0; }

float gpsGetSpeedKmph() { return packetValid ? (lastPacket.speed / 100.0f) : 0.0f; }
float gpsGetAltitude()  { return packetValid ? (lastPacket.altitude / 100.0f) : 0.0f; }
float gpsGetCourse()    { return packetValid ? (lastPacket.course / 100.0f) : 0.0f; }

uint8_t gpsGetHour()   { return packetValid ? lastPacket.hour : 0; }
uint8_t gpsGetMinute() { return packetValid ? lastPacket.minute : 0; }
uint8_t gpsGetSecond() { return packetValid ? lastPacket.second : 0; }
uint16_t gpsGetYear()  { return packetValid ? lastPacket.year : 0; }
uint8_t gpsGetMonth()  { return packetValid ? lastPacket.month : 0; }
uint8_t gpsGetDay()    { return packetValid ? lastPacket.day : 0; }

void gpsSetLedColor(uint8_t r, uint8_t g, uint8_t b) {
  Wire.beginTransmission(C3_I2C_ADDR);
  Wire.write(0x01);
  Wire.write(r); Wire.write(g); Wire.write(b);
  Wire.write(0x01);
  Wire.endTransmission();
}
void gpsSetLedAuto() {
  Wire.beginTransmission(C3_I2C_ADDR);
  Wire.write(0x02);
  Wire.endTransmission();
}

#endif // GPS_USE_I2C_C3


// ============================================
// РЕЖИМ: UART напрямую (стандартный NEO-6M)
// ============================================
#ifdef GPS_USE_UART

#include <TinyGPS++.h>

static TinyGPSPlus       gps;
static HardwareSerial*   gpsSerial = nullptr;

static void sendUbxPoll(uint8_t cls, uint8_t id) {
  uint8_t msg[8] = { 0xB5, 0x62, cls, id, 0x00, 0x00 };
  uint8_t ckA = 0, ckB = 0;
  for (uint8_t i = 2; i < 6; i++) { ckA += msg[i]; ckB += ckA; }
  msg[6] = ckA; msg[7] = ckB;
  gpsSerial->write(msg, 8);
  gpsSerial->flush();
}

static bool waitForUbx(uint8_t cls, uint8_t id, uint16_t timeoutMs) {
  uint32_t start = millis();
  uint8_t state = 0;
  uint8_t rxClass, rxId;
  uint16_t payloadLen, rxCount;
  uint8_t ckA = 0, ckB = 0;
  uint8_t rxBuf[64];

  while (millis() - start < timeoutMs) {
    while (gpsSerial->available()) {
      uint8_t c = gpsSerial->read();
      switch (state) {
        case 0: if (c == 0xB5) state = 1; break;
        case 1: if (c == 0x62) { state = 2; ckA = 0; ckB = 0; } else if (c != 0xB5) state = 0; break;
        case 2: rxClass = c; ckA += c; ckB += ckA; state = 3; break;
        case 3: rxId = c; ckA += c; ckB += ckA; state = 4; break;
        case 4: payloadLen = c; ckA += c; ckB += ckA; state = 5; break;
        case 5: payloadLen |= ((uint16_t)c << 8); ckA += c; ckB += ckA; rxCount = 0;
                state = (payloadLen == 0) ? 7 : 6; break;
        case 6: if (rxCount < sizeof(rxBuf)) rxBuf[rxCount] = c;
                ckA += c; ckB += ckA; rxCount++;
                if (rxCount >= payloadLen) state = 7;
                break;
        case 7: if (c == ckA) state = 8; else state = 0; break;
        case 8: if (c == ckB && rxClass == cls && rxId == id) return true;
                state = 0; break;
      }
    }
    yield();
  }
  return false;
}

void gpsInit() {
  gpsSerial = new HardwareSerial(GPS_UART_NUM);
  gpsSerial->begin(GPS_UART_BAUD, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);
}

bool gpsCheckConnection() {
  if (!gpsSerial) return false;
  while (gpsSerial->available()) gpsSerial->read();
  for (uint8_t i = 0; i < 3; i++) {
    sendUbxPoll(0x0A, 0x04);
    if (waitForUbx(0x0A, 0x04, 300)) return true;
    if (i < 2) delay(100);
  }
  uint32_t start = millis();
  while (millis() - start < 1000) {
    while (gpsSerial->available()) {
      if (gpsSerial->read() == '$') return true;
    }
    yield();
  }
  return false;
}

void gpsUpdate() {
  if (!gpsSerial) return;
  while (gpsSerial->available()) gps.encode(gpsSerial->read());
}

bool gpsHasFix()     { return gps.location.isValid() && gps.location.age() < 5000; }
uint8_t gpsGetSats() { return gps.satellites.isValid() ? gps.satellites.value() : 0; }
float gpsGetLat()    { return gps.location.isValid() ? gps.location.lat() : 0.0f; }
float gpsGetLon()    { return gps.location.isValid() ? gps.location.lng() : 0.0f; }
float gpsGetHdop()   { return gps.hdop.isValid() ? gps.hdop.hdop() : 99.99f; }

uint8_t gpsGetFixQuality() { return gps.location.fixQuality.isValid() ? gps.location.fixQuality.value() : 0; }
uint8_t gpsGetFixMode()    { return gps.location.fixMode.isValid()    ? gps.location.fixMode.value()    : 0; }

float gpsGetSpeedKmph() { return gps.speed.isValid()    ? gps.speed.kmph()    : 0.0f; }
float gpsGetAltitude()  { return gps.altitude.isValid() ? gps.altitude.meters() : 0.0f; }
float gpsGetCourse()    { return gps.course.isValid()   ? gps.course.deg()    : 0.0f; }

uint8_t gpsGetHour()   { return gps.time.isValid() ? gps.time.hour() : 0; }
uint8_t gpsGetMinute() { return gps.time.isValid() ? gps.time.minute() : 0; }
uint8_t gpsGetSecond() { return gps.time.isValid() ? gps.time.second() : 0; }
uint16_t gpsGetYear()  { return gps.date.isValid() ? gps.date.year() : 0; }
uint8_t gpsGetMonth()  { return gps.date.isValid() ? gps.date.month() : 0; }
uint8_t gpsGetDay()    { return gps.date.isValid() ? gps.date.day() : 0; }

void gpsSetLedColor(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b; }
void gpsSetLedAuto() {}

#endif // GPS_USE_UART


// ============================================
// РЕЖИМ: I2C мост SC16IS750
// ============================================
#ifdef GPS_USE_I2C_SC16IS750

#include <Wire.h>
#include <TinyGPS++.h>

#define SC16IS750_ADDR  GPS_SC16IS750_ADDR
#define SC16IS750_XTAL  GPS_SC16IS750_XTAL

#define SC16IS750_RHR   0x00
#define SC16IS750_THR   0x00
#define SC16IS750_IER   0x01
#define SC16IS750_FCR   0x02
#define SC16IS750_LCR   0x03
#define SC16IS750_MCR   0x04
#define SC16IS750_LSR   0x05
#define SC16IS750_DLL   0x00
#define SC16IS750_DLH   0x01

static TinyGPSPlus gps;

static bool sc16is750_writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(SC16IS750_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static uint8_t sc16is750_readReg(uint8_t reg) {
  Wire.beginTransmission(SC16IS750_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFF;
  if (Wire.requestFrom((uint8_t)SC16IS750_ADDR, (uint8_t)1) != 1) return 0xFF;
  return Wire.read();
}

static void sc16is750_initUart(uint32_t baud) {
  sc16is750_writeReg(SC16IS750_LCR, 0x83);
  uint16_t divisor = (uint16_t)(SC16IS750_XTAL / (baud * 16UL));
  sc16is750_writeReg(SC16IS750_DLL, divisor & 0xFF);
  sc16is750_writeReg(SC16IS750_DLH, (divisor >> 8) & 0xFF);
  sc16is750_writeReg(SC16IS750_LCR, 0x03);
  sc16is750_writeReg(SC16IS750_FCR, 0x07);
  sc16is750_writeReg(SC16IS750_MCR, 0x00);
}

static inline bool sc16is750_dataReady() {
  return (sc16is750_readReg(SC16IS750_LSR) & 0x01) != 0;
}

static inline uint8_t sc16is750_readByte() {
  return sc16is750_readReg(SC16IS750_RHR);
}

void gpsInit() {
  Wire.begin();
  sc16is750_initUart(GPS_UART_BAUD);
}

bool gpsCheckConnection() {
  uint8_t lsr = sc16is750_readReg(SC16IS750_LSR);
  if (lsr == 0xFF) {
    Serial.println("[GPS] SC16IS750 не отвечает по I2C");
    return false;
  }
  sc16is750_writeReg(SC16IS750_FCR, 0x07);
  uint32_t start = millis();
  while (millis() - start < 1500) {
    if (sc16is750_dataReady()) {
      Serial.println("[GPS] SC16IS750: данные от GPS обнаружены");
      return true;
    }
    delay(10);
  }
  Serial.println("[GPS] SC16IS750 доступен, GPS пока молчит (возможно холодный старт)");
  return (lsr != 0xFF);
}

void gpsUpdate() {
  while (sc16is750_dataReady()) {
    gps.encode(sc16is750_readByte());
  }
}

bool gpsHasFix()     { return gps.location.isValid() && gps.location.age() < 5000; }
uint8_t gpsGetSats() { return gps.satellites.isValid() ? gps.satellites.value() : 0; }
float gpsGetLat()    { return gps.location.isValid() ? gps.location.lat() : 0.0f; }
float gpsGetLon()    { return gps.location.isValid() ? gps.location.lng() : 0.0f; }
float gpsGetHdop()   { return gps.hdop.isValid() ? gps.hdop.hdop() : 99.99f; }

uint8_t gpsGetFixQuality() { return gps.location.fixQuality.isValid() ? gps.location.fixQuality.value() : 0; }
uint8_t gpsGetFixMode()    { return gps.location.fixMode.isValid()    ? gps.location.fixMode.value()    : 0; }

float gpsGetSpeedKmph() { return gps.speed.isValid()    ? gps.speed.kmph()    : 0.0f; }
float gpsGetAltitude()  { return gps.altitude.isValid() ? gps.altitude.meters() : 0.0f; }
float gpsGetCourse()    { return gps.course.isValid()   ? gps.course.deg()    : 0.0f; }

uint8_t gpsGetHour()   { return gps.time.isValid() ? gps.time.hour() : 0; }
uint8_t gpsGetMinute() { return gps.time.isValid() ? gps.time.minute() : 0; }
uint8_t gpsGetSecond() { return gps.time.isValid() ? gps.time.second() : 0; }
uint16_t gpsGetYear()  { return gps.date.isValid() ? gps.date.year() : 0; }
uint8_t gpsGetMonth()  { return gps.date.isValid() ? gps.date.month() : 0; }
uint8_t gpsGetDay()    { return gps.date.isValid() ? gps.date.day() : 0; }

void gpsSetLedColor(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b; }
void gpsSetLedAuto() {}

#endif // GPS_USE_I2C_SC16IS750
