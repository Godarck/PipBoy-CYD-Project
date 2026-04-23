#include "weather_module.h"
#include "config.h"
#include "eeprom_module.h"
#include "rtc_module.h"
#include <WiFi.h>


String weatherLat = DEFAULT_LAT;
String weatherLon = DEFAULT_LON;
bool weatherCelsius = true;

static CurrentWeather currentWeather = {
  "--", "Loading...", "--", 0, 0, WEATHER_EEPROM
};

static unsigned long lastAttempt = 0;
static bool forceUpdate = false;
static time_t eepromDataTime = 0;

void weatherInit() {
  // Пробуем загрузить из EEPROM при старте
  weatherLoadFromEEPROM();
}

CurrentWeather* weatherGetCurrent() {
  return &currentWeather;
}

bool weatherHasData() {
  return (currentWeather.temperature != "--");// && currentWeather.lastUpdate > 0);
}


time_t weatherLastUpdate()
{
  if (currentWeather.lastUpdate)
    return currentWeather.lastUpdate; 
  
}


int weatherGetAgeMinutes() {
  if (!weatherHasData()) return -1;
  
    // Иначе считаем через RTC (для EEPROM или старых данных)
  time_t nowRtc = now();
  time_t dataTime = currentWeather.lastUpdate; // / 1000;
  long diff = (nowRtc - dataTime) / 60;

/*
  // Если данные свежие (меньше часа) и был WiFi - используем millis
  unsigned long millisAge = (now() - currentWeather.lastUpdate) / 60000;
  
  if (currentWeather.source = WEATHER_EEPROM && millisAge < 60) {
    return millisAge;
  }
  if (currentWeather.source != WEATHER_EEPROM && millisAge < 60) {
    return millisAge;
  }
*/  

  
  if (diff < 0) diff = 0;
  if (diff > 999) diff = 999;
  
  return diff;
}

void weatherForceUpdate() {
  if (DEBUGFLAG) Serial.println("[WEATHER] Try force update.");
  forceUpdate = true;
}

// ======================= ПАРСИНГ =======================

// Функция для преобразования стрелки Unicode в градусы
int arrowToDegrees(String arrow) {
  if (arrow.length() < 3) return 0;
  byte b0 = arrow.charAt(0);
  byte b1 = arrow.charAt(1); 
  byte b2 = arrow.charAt(2);
  
  if (b0 == 0xE2 && b1 == 0x86) {
    if (b2 == 0x93) return 0;     // ↓ (N)
    if (b2 == 0x99) return 45;    // ↙ (NE)
    if (b2 == 0x90) return 90;    // ← (E)
    if (b2 == 0x96) return 135;   // ↖ (SE)
    if (b2 == 0x91) return 180;   // ↑ (S)
    if (b2 == 0x97) return 225;   // ↗ (SW)
    if (b2 == 0x92) return 270;   // → (W)
    if (b2 == 0x98) return 315;   // ↘ (NW)
  }
  return 0;
}

int windDirToDegrees(String dir) {
  dir.trim(); // Убираем пробелы
  
  // Для стрелок Unicode
  if (dir == "↓") return 0;      // Север (ветер с севера)
  if (dir == "↙") return 45;     // Северо-Восток
  if (dir == "←") return 90;     // Восток
  if (dir == "↖") return 135;    // Юго-Восток
  if (dir == "↑") return 180;    // Юг
  if (dir == "↗") return 225;    // Юго-Запад
  if (dir == "→") return 270;    // Запад
  if (dir == "↘") return 315;    // Северо-Запад
  
  // Для букв (если используете латинские обозначения)
  if (dir == "N") return 0;
  if (dir == "NE") return 45;
  if (dir == "E") return 90;
  if (dir == "SE") return 135;
  if (dir == "S") return 180;
  if (dir == "SW") return 225;
  if (dir == "W") return 270;
  if (dir == "NW") return 315;
  
  return 0; // Неизвестное направление
}

// Использование:
//String windDir = currentWeather.windDir;  // "↓"
//int deg = windDirToDegrees(windDir);      // 0

static String degreesToArrow(int degrees) {
  if (degrees >= 337 || degrees < 23) return "↓";
  if (degrees >= 23 && degrees < 68) return "↙";
  if (degrees >= 68 && degrees < 113) return "←";
  if (degrees >= 113 && degrees < 158) return "↖";
  if (degrees >= 158 && degrees < 203) return "↑";
  if (degrees >= 203 && degrees < 248) return "↗";
  if (degrees >= 248 && degrees < 293) return "→";
  if (degrees >= 293 && degrees < 337) return "↘";
  return "•";
}

static String weatherCodeToString(int code) {
  switch(code) {
    case 0: return "Clear";
    case 1: return "Mainly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Drizzle";
    case 61: case 63: case 65: return "Rain";
    case 71: case 73: case 75: return "Snow";
    case 80: case 81: case 82: return "Showers";
    case 95: case 96: case 99: return "Storm";
    default: return "Code:" + String(code);
  }
}

static void parseWindWithArrow(String windFull, String& arrow, String& speed) {
  arrow = "";
  speed = "";
  
  for (int i = 0; i < windFull.length(); i++) {
    char c = windFull.charAt(i);
    if (c == '↓' || c == '↑' || c == '←' || c == '→' ||
        c == '↖' || c == '↗' || c == '↘' || c == '↙') {
      arrow += c;
    } else if (c >= '0' && c <= '9') {
      speed = windFull.substring(i);
      speed.replace("km/h", "");
      speed.trim();
      break;
    }
  }
  
  if (arrow.length() == 0) arrow = "→";
}

// ======================= ИСТОЧНИКИ =======================

static int fetchWttr() {
  HTTPClient http;
  String url = "http://wttr.in/" + weatherLat + "," + weatherLon + "?format=%t|%w|%C";
  
  if (DEBUGFLAG) Serial.print("[WEATHER] Wttr: ");
  if (DEBUGFLAG) Serial.println(url);
  
  http.begin(url);
  http.setTimeout(WEATHER_TIMEOUT_WTTR);
  
  int code = http.GET();
  
  if (code == 200) {
    String payload = http.getString();
    if (DEBUGFLAG) Serial.printf("[WEATHER] Parce from: %s", String(payload));
    
    int sep1 = payload.indexOf('|');
    int sep2 = payload.indexOf('|', sep1 + 1);
    int sep3 = payload.indexOf('|', sep2 + 1);

    if (sep1 > 0 && sep2 > sep1 && sep3 > sep2) {
      // Температура
      String tempStr = payload.substring(0, sep1);
      tempStr.replace("+", "");
      tempStr.replace("°C", "");
      tempStr.trim();
      
      float temp = tempStr.toFloat();
      if (!weatherCelsius) temp = temp * 9.0/5.0 + 32;
      currentWeather.temperature = String((int)round(temp));
      
      // Ветер
      String windFull = payload.substring(sep1 + 1, sep2); // "↓ 25km/h"
      // Извлекаем Unicode стрелку (3 байта)
      String arrow = windFull.substring(0, 3);
      currentWeather.windDir = arrowToDegrees(arrow);

      String speed = "";
      for (int i = 0; i < windFull.length(); i++)
      {
        char c = windFull.charAt(i);
        if (c >= '0' && c <= '9')
        {
          speed = windFull.substring(i);
          speed.replace("km/h", "");
          speed.trim();
          break;
        }
      }
      currentWeather.wind = speed;
      
      // Условия
      currentWeather.condition = payload.substring(sep2 + 1);
      currentWeather.condition.trim();
      if (currentWeather.condition.length() > 14) {
        currentWeather.condition = currentWeather.condition.substring(0, 14);
      }

      String degStr = payload.substring(sep3 + 1);
      //degStr.trim();
      //currentWeather.windDir = degStr.toInt();
      
      currentWeather.source = WEATHER_PRIMARY;
      currentWeather.lastUpdate = now();
      if (DEBUGFLAG) Serial.println("[WEATHER] Wttr OK");
    } else {
      code = -100; // Parse error
    }
  }
  
  http.end();
  return code;
}

static int fetchOpenMeteo() {
  HTTPClient http;
  String url = "http://api.open-meteo.com/v1/forecast?latitude=" + 
               weatherLat + "&longitude=" + weatherLon + "&current_weather=true";
  
  if (DEBUGFLAG) Serial.print("[WEATHER] Open-Meteo: ");
  if (DEBUGFLAG) Serial.println(url);
  
  http.begin(url);
  http.setTimeout(WEATHER_TIMEOUT_OPENMETEO);
  
  int code = http.GET();
  
  if (code == 200) {
    String payload = http.getString();
    if (DEBUGFLAG) Serial.printf("[WEATHER] Parce from: %s", String(payload));

    StaticJsonDocument<512> doc;
    if (!deserializeJson(doc, payload)) {
      float temp = doc["current_weather"]["temperature"];
      int wind = doc["current_weather"]["windspeed"];
      int wCode = doc["current_weather"]["weathercode"];
      int wDir = doc["current_weather"]["winddirection"];
      
      if (!weatherCelsius) temp = temp * 9.0/5.0 + 32;
      
      currentWeather.temperature = String((int)round(temp));
      currentWeather.wind = String(wind);
      currentWeather.windDir = wDir;
      currentWeather.condition = weatherCodeToString(wCode);
      
      currentWeather.source = WEATHER_OPENMETEO;
      currentWeather.lastUpdate = now();
      if (DEBUGFLAG) Serial.println("[WEATHER] Open-Meteo OK");
    } else {
      code = -101; // JSON error
    }
  }
  
  http.end();
  return code;
}

// ======================= EEPROM =======================

bool weatherLoadFromEEPROM() {
  WeatherData data;
  
  // Очистка перед чтением
  memset(&data, 0, sizeof(WeatherData));
  
  if (!eepromReadSlot(0, (uint8_t*)&data)) {
    if (DEBUGFLAG) Serial.println("[WEATHER] read weather slot 0 --- failed");
    return false;
  }
  
  // Проверка valid
  if (data.valid != 1) {
    if (DEBUGFLAG) Serial.print("[WEATHER] weather data in EEPROM invalid, valid = ");
    if (DEBUGFLAG) Serial.println(data.valid);
    return false;
  }
  
  // Проверка checksum
  uint8_t calc = 0;
  uint8_t* ptr = (uint8_t*)&data;
  for (int i = 0; i < 31; i++) {
    calc ^= ptr[i];
  }
  
  if (calc != data.checksum) {
    #if DEBUGFLAG
    Serial.print("[WEATHER] EEPROM  checksum mismatch: calc =");
    Serial.print(calc);
    Serial.print(" , stored =");
    Serial.println(data.checksum);
    #endif
    return false;
  }
  
  // Загрузка данных
  float temp = data.temperature / 10.0;
  currentWeather.temperature = String((int)round(temp));

  currentWeather.condition = String(data.condition);
  currentWeather.condition = currentWeather.condition.substring(0, 14);

  currentWeather.wind = String(data.wind);
  currentWeather.wind = currentWeather.wind.substring(0, 8);

  currentWeather.windDir = data.windDir;
  
  //eepromDataTime = data.timestamp;
  // Восстановление времени
  time_t rtcNow = now();
  time_t dataTime = data.timestamp;

  if (dataTime > rtcNow) {

   rtc.adjust(DateTime(year(dataTime), month(dataTime), day(dataTime), 
                     hour(dataTime), minute(dataTime), second(dataTime)));
    Serial.print("[WEATHER] Date in RTC older then in Weather backup. Adjust RTC.\n");
  }

  long ageSec = (rtcNow - dataTime);// / 1000;
  if (ageSec < 0) ageSec = 0;
  
  //currentWeather.lastUpdate = ageSec / 60;
  currentWeather.lastUpdate = dataTime; //millis() - (ageSec * 1000);
  //if (currentWeather.lastUpdate > millis()) currentWeather.lastUpdate = 0;
  
  currentWeather.source = WEATHER_EEPROM;
  #if DEBUGFLAG
  Serial.print("[WEATHER] EEPROM loaded OK, Update age = ");
  Serial.print(ageSec);
  Serial.print(" sec, ");
  Serial.print(ageSec / 60);
  Serial.println(" min.");
    char stringwu[32];
    snprintf(stringwu, sizeof(stringwu), "%02d.%02d.%04d %02d:%02d:%02d", day(dataTime), month(dataTime), year(dataTime), hour(dataTime), minute(dataTime), second(dataTime));
    Serial.printf("Update date was: %s\n", stringwu); 
  #endif
  return true;
}


void weatherSaveToEEPROM() {
  WeatherData data;
  
  // Очистка всей структуры перед использованием!
  memset(&data, 0, sizeof(WeatherData));
  
  data.temperature = (int16_t)(currentWeather.temperature.toFloat() * 10);
  
  // Безопасное копирование строк с гарантированным \0
  strncpy(data.condition, currentWeather.condition.c_str(), 13);
  data.condition[13] = '\0';
  
  strncpy(data.wind, currentWeather.wind.c_str(), 7);
  data.wind[7] = '\0';
  
  data.windDir = currentWeather.windDir;
  // windDir — стрелка (1 символ + \0)
 // if (currentWeather.windDir.length() > 0) {
 //   data.windDir[0] = currentWeather.windDir.charAt(0);
 // } else {
 //   data.windDir[0] = '-';
 // }
  //data.windDir[1] = '\0';
  
  data.timestamp = (uint32_t)now();
  data.valid = 1;
  
  // Checksum: XOR первых 31 байт (все кроме checksum itself)
  data.checksum = 0;
  uint8_t* ptr = (uint8_t*)&data;
  for (int i = 0; i < 31; i++) {
    data.checksum ^= ptr[i];
  }
  
  if (eepromWriteSlot(0, (uint8_t*)&data)) {
   if (DEBUGFLAG)  Serial.println("[WEATHER] Weather saved to EEPROM");
  }
}


// ======================= ОСНОВНАЯ ФУНКЦИЯ =======================

void weatherUpdate() {
  // Проверяем необходимость обновления
  unsigned long elapsed = now() - currentWeather.lastUpdate;
  bool needUpdate = forceUpdate || 
                    (elapsed > WEATHER_UPDATE_INTERVAL) ||
                    (currentWeather.lastUpdate == 0);
  
  if (!needUpdate) return;
  
  forceUpdate = false;
  lastAttempt = millis();
  if (DEBUGFLAG) Serial.println("[WEATHER] Try to update weather data.");
  // Нет WiFi - пробуем EEPROM
  if (WiFi.status() != WL_CONNECTED) {
    if (DEBUGFLAG) Serial.println("[WEATHER] No WiFi, loading from EEPROM");
    weatherLoadFromEEPROM();
    return;
  }
  if (DEBUGFLAG) Serial.println("[WEATHER] Trying wttr ....");
  // Пробуем wttr.in
  int result = fetchWttr();
  
  // Если ошибка (включая -11 timeout) - пробуем Open-Meteo
  if (result != 200) {
    #if DEBUGFLAG
    Serial.print("[WEATHER] Wttr failed. Error: ");
    Serial.println(result);
    Serial.println("[WEATHER] Trying Open-Meteo .... ");
    #endif
    result = fetchOpenMeteo();
  }
  
  // Если успех - сохраняем в EEPROM
  if (result == 200 && currentWeather.temperature != "--") {
    weatherSaveToEEPROM();
  } else {
    // Все источники недоступны - загружаем из EEPROM
    #if DEBUGFLAG
    Serial.print("[WEATHER] OpenMeteo failed. Error: ");
    Serial.println(result);
    Serial.println("[WEATHER] All sources failed, loading EEPROM");
    #endif
    weatherLoadFromEEPROM();
  }
}
