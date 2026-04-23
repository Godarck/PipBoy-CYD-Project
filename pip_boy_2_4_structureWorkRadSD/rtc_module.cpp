#include "rtc_module.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiUdp.h>

RTC_DS1307 rtc;
bool rtcFound = false;
bool ntpSynced = false;
time_t lastNtpSync = 0;

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
  Wire.begin(RTC_SDA, RTC_SCL);
  
  Wire.beginTransmission(RTC_ADDRESS);
  if (Wire.endTransmission() != 0) {
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
