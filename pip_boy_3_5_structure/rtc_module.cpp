#include "rtc_module.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <GPSModule.h>

RTC_DS1307 rtc;
bool rtcFound = false;
bool ntpSynced = false;
time_t lastNtpSync = 0;
bool gpsTimeSynced = false;
unsigned long lastGpsTimeSync = 0;

//class GPSModule;
extern GPSModule gps;

// Часовой пояс (настрой под себя)
TimeChangeRule myStandardTime = {"GMT", First, Sun, Nov, 2, 0};
TimeChangeRule myDaylightSavingsTime = {"CDT", Second, Sun, Mar, 2, GMT_SET * 60};  // +3 = GMT +3
Timezone myTZ(myStandardTime, myDaylightSavingsTime);
TimeChangeRule *tcr;

static WiFiUDP Udp;
static const int NTP_PACKET_SIZE = 48;
static byte packetBuffer[NTP_PACKET_SIZE];
static unsigned int localPort = 8888;

void debugPrint(const char* label, int value) {
  #if DEBUGFLAG
  Serial.print(label);
  Serial.print("=");
  Serial.println(value);
  #endif
}

void rtcInit() {
  
  Wire.beginTransmission(RTC_ADDRESS);
    int wrtrs = Wire.endTransmission();

    Serial.printf("[WIRE] RTC wire = %d\n", wrtrs);
  if (wrtrs != 0) {
    if (DEBUGFLAG) Serial.println("RTC not found!");
    rtcFound = false;
    return;
  }
  
  if (!rtc.begin()) {
    rtcFound = false;
    return;
  }
  
  rtcFound = true;
  
 // if (rtc.lostPower()) {
  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //}
  
  DateTime rtcNow = rtc.now();
  setTime(rtcNow.unixtime());
  #if DEBUGFLAG
  Serial.print("[RTC]     OK DATE and TIME = ");
  Serial.print(rtcNow.year());
  Serial.print("-");
  Serial.print(rtcNow.month());
  Serial.print("-");
  Serial.print(rtcNow.day());
  Serial.print(" ");
  Serial.print(rtcNow.hour());
  Serial.print(":");
  Serial.print(rtcNow.minute());
  Serial.print(" WORK GMT: ");
  Serial.print(GMT_SET);
  Serial.println("");
  #endif
}

void rtcSyncFromModule() {
  if (!rtcFound) return;
  static unsigned long lastSync = 0;
  if (millis() - lastSync > 60000) {
    DateTime rtcNow = rtc.now();
    setTime(rtcNow.unixtime());
    lastSync = millis();
  }
}

void rtcSaveToModule() {
  if (!rtcFound) return;
  time_t utc = now();
  rtc.adjust(DateTime(year(utc), month(utc), day(utc), 
                     hour(utc), minute(utc), second(utc)));
}


void syncTimeFromGPS() {
    static unsigned long lastMessage = 0;
    // Не чаще раза в минуту (GPS время не дрифтует, нет смысла чаще)
    if ((millis() - lastGpsTimeSync < 60000) || gpsTimeSynced) return;
    
    // GPS должен отдавать хотя бы время (холодный старт тоже подходит)
    if (!gps.hasTime()) return;
    uint8_t h, m, s, d, mo, y;
    gps.getTime(h, m, s);
    gps.getDate(d, mo, y);
    
    // Если RMC ещё не пришёл — дата будет 0.0.0, ждём.
    if (y == 0 && d == 0 && mo == 0) 
    {

      if (DEBUGFLAG) 
      {
        if ((millis() - lastMessage < 10000))
        {
          lastMessage = millis();
          Serial.println("[RTC] Waiting date from GPS...");
        }
      }
      return;
    }
    
    int fullYear = y + 2000;  // NMEA: 26 → 2026
    
    // Защита от мусора
    if (fullYear < 2024 || fullYear > 2035) return;
    if (mo < 1 || mo > 12 || d < 1 || d > 31) return;
    if (h > 23 || m > 59 || s > 59) return;
    
    // Собираем UTC time_t через RTClib DateTime
    DateTime gpsDt(fullYear, mo, d, h, m, s);
    time_t gpsUtc = gpsDt.unixtime();
    
    // Устанавливаем системное время (Time.h хранит UTC)
    setTime(gpsUtc);
    
    // Сразу пишем в RTC, чтобы время сохранилось при перезагрузке
    if (rtcFound) {
        rtc.adjust(gpsDt);
        rtcSaveToModule();
    }
    if (currentScreen == 1)
    {
      lastScreen = -1;
      needUpdateTimeScreen = true;
    }
    gpsTimeSynced = true;
    lastGpsTimeSync = millis();
    
    if (DEBUGFLAG) Serial.printf("[RTC] GPS Время скорректировано: %02d.%02d.%04d %02d:%02d:%02d UTC\n",
                  d, mo, fullYear, h, m, s);
}


void sendNTPpacket(IPAddress &address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t rtcGetNtpTime() {
  if (WiFi.status() != WL_CONNECTED) return 0;
  
  IPAddress ntpServerIP;
  while (Udp.parsePacket() > 0);
  
  WiFi.hostByName(NTP_SERVER, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long secsSince1900;
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + 1;
    }
  }
  return 0;
}

void rtcSyncNtpIfNeeded() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  static bool udpStarted = false;
  if (!udpStarted) {
    Udp.begin(localPort);
    udpStarted = true;
  }
  
  if (!ntpSynced || (now() - lastNtpSync > NTP_SYNC_INTERVAL)) {
    time_t ntpTime = rtcGetNtpTime();
    if (ntpTime != 0) {
      setTime(ntpTime);
      lastNtpSync = now();
      ntpSynced = true;
      rtcSaveToModule();
      if (DEBUGFLAG) Serial.println("NTP sync OK");
    }
  }
}

bool rtcIsFound() {
  return rtcFound;
}
