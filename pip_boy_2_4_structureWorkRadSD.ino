/*
 * ESP32 2.4" TFT ILI9341 + XPT2046 Touch + RTC DS1307 + AT24C32 EEPROM + WiFi
 * Стилизованный Pip-Boy из Fallout

Flash mod: DIO
partition scheme: NoOta 2mb APP / 2mb SPIFFS
events run: core1
arduino runs: core1
Upload speed: 460800
PSRAm: Enabled

 libs : 
//esp32-audioi2s-master
esp8266audio
time from Michael Margolis
HTTPClient
ArduinoJson
// ???? not use ogg lib https://github.com/pschatzmann/codec-ogg

password default for wifi in wifi_module.cpp
Default folder from SD card for mp3 in RadioModule.cpp
in RadioModule.cpp pin 26 for autput audio stream

 ====== components =========
 CYD: 2.4 inch ESP32-2432S024R with Resistive touch (Only type-c usb connector, RGB led in Front near display. )
 RTC + EEPROM :   Tyni RTC I2C module DS1307
connectors:       JST 1.25 4pin - for i2c bus
                  JST 1.25 2pin (2 pcs)  - for dinamic and battery
 Для работы часов нужен модульс флэшкой памяти (на модуле должно быть две 8 ногих микросхемы)/ либо флэшка памяти отдельно i2c
 

 динамик ( можно от мобильника, например с старых айфонов)
 модуль сенсора пульса (опционально) пока не реализовано
 флэшка microSD (до 32 гб, чем меньше тем лучше. оптимально - 8 гб) FAT32. С нее считывается музыка локально. Радиостанции из фалаут можно найти отдельно.


пароль вай вай запоминает последний введенный,
координаты GPS и выбор температры( Celsius - Farengheit) запоминает последние введенные

папку с MP3 файлами так же запоминает введенную (по умолчанию SD:/mp3/)
====== на главном экране ======
слева в углу 3 состояния:
в рамке - активно
wifi - подключен ли WiFi
Rad - идет ли воспроизведение музыки
W индикатор погоды:
W Err - данных о погоде нет
W [E] - данные считанные из EEPROM
W [O] - Данные актуальные из OpenMeteo
W [W] - Данные актуальные из WTTR




HP - индикатор пульса
HP - зависит от времени суток (с 8 утра до 8 вечера уменьшается с 420 до 80) Потом восстанавливается до максимума к 03:00
AP - зависит от времени суток (с 8 утра до 13:00 уменьшается до 60, потом до 5:00 уменьшается до 30.
 Потом с 5:00 до 5:30 уменьшается до 0 каждую минуту отнимаеся 1) Потом восстанавливается до максимума каждые 5 минут.


 */
// Include SD




#include "config.h"
#include "rtc_module.h"
#include "eeprom_module.h"
#include "wifi_module.h"
#include "weather_module.h"
#include "ui_module.h"
#include "radio.h"
//#include "radio_module.h"

#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <TimeLib.h>
#include <SD.h>
#include <FS.h>

// ======================= BITMAPS =======================
#include <bitmaps.h>
#include <bitmaps180.h>
#include <labelpip317x47.h>
#include <logovt180x80.h>
#include <weather_icons.h>
const unsigned char* vaultBoyFrames[7] = {myBitmap1, myBitmap2, myBitmap3, myBitmap4, myBitmap5, myBitmap6, myBitmap7};
const unsigned char* specialBMP[17] = {radioboy, mainStat, mainStat2, pipup, sp1, sp2, sp3, sp4, sp5, sp6, sp7, sp8, sp9, sp10, sp11, sp12};

// ======================= ЧАСЫ =======================
#include "Digit.h"
Digit *digs[6];
int colons[2];
int ampm[2];
bool ispm;
bool clockInitialized = false;
time_t prevDisplay = 0;

// ======================= ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =======================

char StandartWiFiPass[32] = "c9608b67b936";

// struct fo 32 bytes for eeprom
struct BackUpDataGPS {
  char GPS_LAT[15];        // 15 байт
  char GPS_LON[15];              // 15 байт
  uint8_t icCels; 
} __attribute__((packed));   // <-- БЕЗ ПАДДИНГА!

struct BackUpDataWiFi {
  char WiFi_Pass[32];           // 32 байт
} __attribute__((packed));   // <-- БЕЗ ПАДДИНГА!

struct BackUpDataFolder {
  char SDFolder[32];           // 32 байт
} __attribute__((packed));   // <-- БЕЗ ПАДДИНГА!

// Калибровка тачскрина (rotation 1)
uint16_t calData[5] = { 294, 3577, 349, 3453, 0 };

// Настройки отображения часов
const bool SHOW_24HOUR = true;
const bool SHOW_AMPM = false;
const bool NOT_US_DATE = true;

// Параметры шрифта часов
int clockFont = 1;
int clockSize = 6;
int clockDatum = TL_DATUM;
uint16_t clockBackgroundColor = TFT_BLACK;
uint16_t clockFontColor = TFT_GREEN; // Зелёный для Pip-Boy стиля!
int timeY = 40; // Позиция часов на экране

int prevDay = 0;


TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
 //SPIClass SDSPI(VSPI);


int currentScreen = 0;
int lastScreen = -1;  //mp3/
int vaultFrame = 1;
const int bitmapStartX = ((320 - 180) / 2);  // Фиксированное значение для rotation 1


// General screen
int ButtonScreen2 = 0;
bool weatherSettingsActive = false;
bool needUpdateScreenWeather = false;
bool editlat = false;
bool editlon = false;

int currentHP = 420;
int currentAP = 420;
int apMax = 420;   // Максимум AP (можно менять, например, при прокачке персонажа)
int hpMax = 420;   // Максимум HP


// === RADIO SETTINGS ===
int radioPlaySource = 0;        // 0 = SD (default), 1 = WiFi, 2 = Ext
String radioSDFolder = "/mp3";
bool radioSettingsActive = false;
bool sdCardInitialized = false;

int RadStationInd = 1;
int RadVolume = 50;

std::vector<String> sdPlaylist;
int  currentSDTrack      = 0;
bool sdPlaylistLoaded    = false;

SemaphoreHandle_t radioMutex = NULL;


SPIClass SDSPI(VSPI);

/*-------- DEBUGGING ----------*/
void Debug(String label, uint8_t val)
{
  Serial.print(label);
  Serial.print("=");
  Serial.println(val);
}

// ======================= ПРОТОТИПЫ ФУНКЦИЙ =======================
void initI2C();
bool LoadBackUpFromEPPR();
void initStartUp();
void drawWindArrow();
void drawScanlines();
void drawScanlinesButtons(int16_t xS, int16_t yS, int16_t hS, int16_t wS);
void drawPipBoyScreen();
void drawPipBoyScreen1();
void drawPipBoyScreen2();
void drawPipBoyScreen3();
void drawPipBoyScreen4();
void drawButtonsScreen4();
void drawRadioSetButtons();
void updateHPAP();

void drawTabButtons();
void drawStatusButton(bool active);
void drawSpecialButton(bool active);
void drawSkillsButton(bool active);
void drawWeatherButton(bool active);
void drawGeneralButton(bool active);
void drawVaultBoy(int16_t cx, int16_t cy, int8_t frame);
void updateWeatherTimeDisplay();
void drawWiFiScreen();
void HandleButtonsScreen4(uint16_t x, uint16_t y);
void scanWiFiNetworks();
void connectToWiFi(const char* ssid, const char* password);
void drawWeatherSettings();
void handleWeatherSettingsTouch(uint16_t x, uint16_t y);

// Клавиатура
void initKeyboard();
void drawKeyboard();
void drawKey(Key* k, bool pressed);
void redrawInputField();
void showKeyboard(const char* placeholder);
void hideKeyboard();
const char* getKeyboardInput();
bool handleKeyboardTouch(uint16_t x, uint16_t y);
void clearKeyboardInput();

// Часы
void SetupDigits();
void CalculateDigitOffsets();
void DrawColons();
void DrawAmPm();
void DrawDigitsAtOnce();
void DrawADigit();
void fillLinesSprite();
void DrawDigitsOneByOne();
void ParseDigits(time_t utc);
void DrawDate(time_t utc);

// погода
bool isDayTime();
void drawWeatherIcon(int x, int y, String condition, uint16_t color);
void drawCurrentWeatherIcon(int x, int y, uint16_t color);
void drawWeatherIconCentered(int x, int y, String condition, uint16_t color);
void drawCurrentWeatherIconCentered(int x, int y, uint16_t color);

// radio
// Прототипы (добавь в раздел ПРОТОТИПЫ ФУНКЦИЙ
void drawRadioSettings();
void handleRadioSettingsTouch(uint16_t x, uint16_t y);
bool checkSDPath(const char* path);
void radioSetSDFolder(const String& folder);
int  radioScanSDFolder();
int  radioGetSDTrackCount();
const char* radioGetSDTrackName();
//void loadSDPlaylist();

void radioStartTask();
void radioInit();
//void radioLoop();
void radioPlayStation(int index);
bool radioNextStation();
bool radioPrevStation();
bool radioNextTrack();
bool radioPrevTrack();
void radioPlay();
void radioPause();
void radioSetVolume(uint8_t vol);  // 0-100
uint8_t radioGetVolume();
const char* radioGetStationName();
bool radioIsPlaying();
int radioGetStationIndex();
//bool radioHasTimeout();
//bool radioHasInvalidFormat();
void UpdateMetaData();
void handleRadioSetButtons(uint16_t x, uint16_t y);

// WiFi
void wifiInit();
void scanWiFiNetworks();
void drawWiFiScreen();
void handleWiFiTouch(uint16_t x, uint16_t y);
void connectToWiFi(const char* ssid, const char* password);
void wifiDisconnect();
bool wifiIsConnected();

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Pip-Boy starting...");
  
/*
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
*/

  //SPIClass SDSPI(VSPI);
  SDSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS); // SDSPI.begin(SCLK, MISO, MOSI);
  //SDSPI.setFrequency(80000000);
  //SD.begin(5, SDSPI);
  if (SD.begin(SD_CS, SDSPI, 1000000)) {                // CS = IO5 (по твоей схеме)
    sdCardInitialized = true;
    Serial.println("SD Card Initialized 1MHz");
  } else if (SD.begin(SD_CS, SDSPI, 2000000))
  {                
    sdCardInitialized = true;
    Serial.println("SD Card Initialized 2MHz");
  }
  else {
    sdCardInitialized = false;
    Serial.println("SD Card mount failed ERROR");
  }

  if (sdCardInitialized)
  {
      uint8_t cardType = SD.cardType();

      if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return;
      }
        Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  }
  
  
  // TFT
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
  
  tft.init();
  tft.setSwapBytes(false);
  tft.setRotation(1);
    // Инициализация модулей
  initStartUp();

  tft.setTouch(calData);
  
  
  // Спрайт для часов
  tft.setTextFont(clockFont);
  tft.setTextSize(clockSize);
  tft.setTextDatum(clockDatum);

  sprite.setRotation(1);
  sprite.setTextFont(clockFont);
  sprite.setTextSize(clockSize);
  sprite.setTextDatum(clockDatum);
  sprite.createSprite(tft.textWidth("8"), tft.fontHeight());
  sprite.setTextColor(clockFontColor);//, clockBackgroundColor);
  

  
  // Заставка
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  tft.drawBitmap(1, 20, labelpip317x47, 317, 47, TFT_GREEN);
  
  for (int i = 0; i < 7; i++) {
    tft.fillRect(100, 100, 128, 64, TFT_BLACK);
    drawScanlinesButtons(100, 100, 64, 128);
    tft.drawBitmap(100, 100, vaultBoyFrames[i], 128, 64, TFT_GREEN);
    delay(250);
  }
  
  delay(2000);
  
  // Первый экран
  drawPipBoyScreen();
  drawTabButtons();
  
  Serial.println("Pip-Boy ready!");
}

// ======================= LOOP =======================
void loop() {
  uint16_t xTouch, yTouch;


  //radioLoop();
  //radioLoop();

  // Обновление времени погоды каждую минуту
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 60000) {
    lastTimeUpdate = millis();
    if (!weatherHasData() ) 
    {
      weatherUpdate();
    }
    
    if (currentScreen == 3 && weatherHasData()) {
      if (needUpdateScreenWeather) drawPipBoyScreen3();
      updateWeatherTimeDisplay();
      if (weatherGetAgeMinutes() > 15)
      {
        weatherForceUpdate();
        weatherUpdate();
        needUpdateScreenWeather = true;
      }
    }
    
    if (currentScreen == 0)
    {
        updateHPAP();
        tft.setTextSize(1);
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(TFT_GREEN);
      if (WiFi.status() == WL_CONNECTED) 
      {
        tft.drawRect(9, 35, 29, 15, TFT_GREEN);
        tft.setCursor(12, 38);
        //tft.fillRect(9, 35, 29, 15, TFT_GREEN);
        tft.print("WiFi");
      }else
      {
        tft.fillRect(9, 35, 29, 15, TFT_BLACK);
        drawScanlinesButtons(9, 35, 18, 29);
        tft.setCursor(12, 38);
        tft.print("WiFi");
      }

      if (radioIsPlaying()) 
      {
        tft.drawRect(9, 55, 29, 15, TFT_GREEN);
        tft.setCursor(12, 58);
        tft.print("RAD");
        //tft.fillRect(9, 35, 29, 15, TFT_GREEN);
      }else
      {
        tft.fillRect(9, 55, 29, 15, TFT_BLACK);
        drawScanlinesButtons(9, 55, 18, 29);
        tft.setCursor(12, 58);
        tft.print("RAD");
      }
      tft.setCursor(12, 78);

      tft.fillRect(9, 75, 35, 15, TFT_BLACK);
      drawScanlinesButtons(9, 75, 18, 35);
      CurrentWeather* w = weatherGetCurrent();
      String src = (w->source == WEATHER_PRIMARY) ? "[W]" : 
               (w->source == WEATHER_OPENMETEO) ? "[O]" : "[E]";
      //tft.drawString(src, 310, 35);
      if ((src == "[W]") || (src == "[O]"))
      {
        src = "W " + src;
        tft.drawRect(9, 75, 35, 15, TFT_GREEN);
        tft.print(src);
      }
      else
      {
        if (eepromFound)
          tft.print("W [E]");
        else
        {
           if (!weatherHasData()) 
            tft.print("ERR W");
          else
            tft.print("ERR E");
        }
          
      }
    }

    if (currentScreen == 2)
    {
       
        Serial.println("Radio screen working..");
        UpdateMetaData();
    }
      
  }
  
  // Автообновление погоды каждые 15 минут
  static unsigned long lastWeatherCheck = 0;
  if (millis() - lastWeatherCheck > WEATHER_UPDATE_INTERVAL) {
    lastWeatherCheck = millis();
    weatherUpdate();
    if (currentScreen == 3) {
      drawPipBoyScreen3();
    }
  }

/*
  if (!weatherHasData())
  {
    if (WiFi.status() = WL_CONNECTED)) 
      weatherUpdate();
    else
      weatherLoadFromEEPROM();
  }
 */

  // NTP синхронизация
  rtcSyncNtpIfNeeded();
  rtcSyncFromModule();
  
  // Обработка тача
  if (tft.getTouch(&xTouch, &yTouch)) {
    handleTouch(xTouch, yTouch);
  }
  
  // Анимация часов
  if (currentScreen == 1 && clockInitialized && timeStatus() != timeNotSet) {
    time_t current = now();
    if (current != prevDisplay) {
      digitalWrite(LED_R, LOW);
      delay(30);
      digitalWrite(LED_R, HIGH);
      prevDisplay = current;
      ParseDigits(prevDisplay);
      DrawDigitsOneByOne();
      DrawDate(prevDisplay);
      DrawColons();
      DrawAmPm();
    }
    delay(100);
  }
  
  // Анимация курсора клавиатуры
  if (keyboardActive) {
    static unsigned long lastCursor = 0;
    if (millis() - lastCursor > 500) {
      lastCursor = millis();
      int textW = tft.textWidth(inputBuffer);
      pcursor = !pcursor;
      if (pcursor)
        tft.drawFastVLine(KEYBOARD_X + 63 + textW, KEYBOARD_Y + 32, 14, TFT_GREEN);
      else
        tft.drawFastVLine(KEYBOARD_X + 63 + textW, KEYBOARD_Y + 32, 14, TFT_BLACK);
    }
  }
  
  // Обработка Enter с клавиатуры
  if (KeyboardEnter) {
    if (wifiWaitingForPassword) {
      wifiWaitingForPassword = false;
      const char* inputWPs = getKeyboardInput();
      memcpy(StandartWiFiPass, inputWPs, 32);
      SaveBackUpToEPPR();
      connectToWiFi(wifiTargetSSID, inputWPs);
      delay(100);
    }
    if(weatherSettingsActive)
    {
      if (editlat) 
      {
        editlat = false;
        weatherLat = String(getKeyboardInput());
        SaveBackUpToEPPR();
      }
      if (editlon) 
      {
        editlon = false;
        weatherLon = String(getKeyboardInput());
        SaveBackUpToEPPR();
      }
      drawWeatherSettings();
    }
    
    
  
    if (radioSettingsActive) {
      const char* input = getKeyboardInput();
      if (input != nullptr && strlen(input) > 0) {
        radioSDFolder = String(input);
        
        // Автоматическое исправление пути
        if (!radioSDFolder.startsWith("/")) radioSDFolder = "/" + radioSDFolder;
        if (!radioSDFolder.endsWith("/")) radioSDFolder += "/";
        SaveBackUpToEPPR();
      }
      
      drawRadioSettings();   // перерисовываем с новой проверкой
    }
    KeyboardEnter = false;
  }
}

void UpdateMetaData()
{
  if (currentScreen == 2)
    {
      tft.setTextColor(TFT_GREEN);
      tft.setTextDatum(TC_DATUM);
      if (WiFi.status() == WL_CONNECTED && radioPlaySource == 1)
      {
        tft.fillRect(0, 42, 320, 102, TFT_BLACK);  // очистка области      
        drawScanlinesButtons(0, 42, 102, 320);
        tft.setTextSize(2);

        //String RadioInfo = "Radio: " + String(radioGetStationName());
        String RadioInfo = String(radioGetStationName());
        
        tft.drawString(RadioInfo, 160, 42);
        if (radioIsPlaying())
          {
            tft.drawString(radioGetCurrentArtist(), 160, 70);
            tft.drawString(radioGetCurrentTitle(), 160, 100);
          }
        //tft.drawString(radioGetCurrentSong(), 160, 100);
      }

      if (radioPlaySource == 0 )
      {
        
          tft.fillRect(0, 42, 320, 102, TFT_BLACK);  // очистка области      
          drawScanlinesButtons(0, 42, 104, 320);
          //if (radioIsPlaying())
          tft.setTextSize(2);
          tft.drawString(radioGetSDTrackName(), 160, 42);
          if (radioIsPlaying())
          {
          tft.drawString(radioGetCurrentArtist(), 160, 70);    
          tft.drawString(radioGetCurrentTitle(), 160, 100);
          }
      }

        tft.setTextSize(1);
        String VolumeInfo = "Volume: " + String(radioGetVolume());
        int widthtext = tft.textWidth(VolumeInfo);
                  tft.fillRect((320 / 2) - (widthtext/2), 148, widthtext, 10, TFT_BLACK);
                  drawScanlinesButtons((320 / 2) - (widthtext/2), 148, 10, widthtext);
                  tft.drawString(VolumeInfo, 320 / 2, 148);
         // }
    }


}

///
void initI2C()
{
  Wire.begin(RTC_SDA,RTC_SCL);
  byte error, address;
  int nDevices = 0;

  tft.println("Scanning for I2C devices ...");
  tft.println(" ");
  delay(20);
  for (address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      tft.printf("I2C device found at address 0x%02X\n", address);
      tft.println(" ");
      delay(20);
      nDevices++;
    } else if (error != 2) {
      tft.printf("Error %d at address 0x%02X\n", error, address);
      tft.println(" ");
      delay(20);
    }
  }
  if (nDevices == 0) {
    tft.println("No I2C devices found");
    tft.println(" ");
    
  }
  delay(200);
}

void initStartUp(){
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  tft.drawBitmap((320 - 180) / 2, (240 - 80)/2, logovt180x80, 180, 80, tft.color565(0, 80, 0));
  delay(150);
  tft.drawBitmap((320 - 180) / 2, (240 - 80)/2, logovt180x80, 180, 80, tft.color565(0, 180, 0));
  delay(350);
  tft.drawBitmap((320 - 180) / 2, (240 - 80)/2, logovt180x80, 180, 80, TFT_GREEN);
  delay(150);
  tft.drawBitmap((320 - 180) / 2, (240 - 80)/2, logovt180x80, 180, 80, tft.color565(0, 180, 0));
   drawScanlines();
  delay(100);
  //tft.color565(0, 180, 0)
  //delay(500);
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  tft.setTextFont(clockFont);
  tft.setTextSize(1);
  tft.setCursor(10, 15);
  delay(250);
  tft.setTextColor(TFT_GREEN);
  tft.print("Loading ");
  
  delay(50);
  rtcInit();
  tft.print(".");
  delay(50);
  eepromInit();
  tft.print(".");
  delay(50);
  wifiInit();
  tft.println(".");
  delay(50);
  tft.println(" ");
  initI2C();
  if (rtcFound)
    tft.println("RTC module --------------- OK");
  else
  {
    tft.print("RTC module ------------- ");
    tft.setTextColor(TFT_RED);
    tft.println("ERROR");
  }
  tft.println(" ");
  delay(200);
  tft.setTextColor(TFT_GREEN);
  
  if (eepromFound)
  {
    tft.println("EEPROM module ------------ OK");
    tft.println(" ");
    if (LoadBackUpFromEPPR())
    {
      tft.println("BACKUP LOAD -------------- OK");
    }
    else
    {
      tft.print("BACKUP LOAD -------------- ");
      tft.setTextColor(TFT_RED);
      tft.println("ERROR");
    }
  }
  else
  {
    tft.print("EEPROM module ------------ ");
    tft.setTextColor(TFT_RED);
    tft.println("ERROR");
    tft.setTextColor(TFT_GREEN);
    tft.println(" ");
    tft.print("BACKUP LOAD -------------- ");
    tft.setTextColor(TFT_RED);
    tft.println("ERROR");
  }
  tft.println(" ");
  delay(200);
  tft.setTextColor(TFT_GREEN);
  if (weatherLoadFromEEPROM())
    tft.println("WEATHER backup module ---- OK");
  else
  {
    tft.print("WEATHER backup module ---- ");
    tft.setTextColor(TFT_RED);
    tft.println("ERROR");
  }
  tft.println(" ");
  delay(200);
  tft.setTextColor(TFT_GREEN);
  tft.println("Wi-Fi module ------------- OK");

  tft.println(" ");
  delay(200);
  tft.setTextColor(TFT_GREEN);

  tft.println("KEYBOARD module ---------- OK");


  radioInit();  // Инициализация радио

  tft.println(" ");
  delay(200);
  tft.print("SD card ");
  if (sdCardInitialized)
    tft.println("------------------ OK");
  else
  {
    tft.setTextColor(TFT_RED);
    tft.println("------------------ ERROR");
    tft.setTextColor(TFT_GREEN);
  }
  

    radioStartTask();
  // Тест RGB
  tft.println(" ");
  delay(200);
  tft.setTextColor(TFT_GREEN);
  tft.print("RGB module ---------");
  delay(200);
  tft.print("--");
  digitalWrite(LED_R, LOW); delay(200);
  digitalWrite(LED_R, HIGH); 
  delay(200);
  tft.print("--");
  digitalWrite(LED_G, LOW);  delay(200);
  digitalWrite(LED_G, HIGH); 
  delay(200);
  tft.print("--");
  digitalWrite(LED_B, LOW);  delay(200);
  digitalWrite(LED_B, HIGH);
  delay(200);
  tft.println(" OK");

  //SetupDigits();
  initKeyboard();
  delay(1500);

}

bool LoadBackUpFromEPPR()
{
BackUpDataGPS dataG;
BackUpDataWiFi dataW;
BackUpDataFolder dataF;

bool ErrorB = false;
  // Очистка перед чтением
  memset(&dataG, 0, sizeof(BackUpDataGPS));
  if (!eepromReadSlot(1, (uint8_t*)&dataG)) {
    Serial.println("[EEPROM] read slot backup GPS ---- failed");
    //return false;
    ErrorB = true;
  }
  else
  {
    weatherLat = String(dataG.GPS_LAT);
    weatherLon = String(dataG.GPS_LON);
    if (dataG.icCels == 1 ) weatherCelsius = true; else weatherCelsius = false;
  }

  memset(&dataW, 0, sizeof(BackUpDataWiFi));
  if (!eepromReadSlot(2, (uint8_t*)&dataW)) {
    Serial.println("[EEPROM] read slot backup wifi pass ---- failed");
    ErrorB = true;
  }
  else
  {
    strncpy(StandartWiFiPass, dataW.WiFi_Pass, sizeof(StandartWiFiPass) - 1);
    StandartWiFiPass[sizeof(StandartWiFiPass) - 1] = '\0';
  //StandartWiFiPass = String(dataW.WiFi_Pass);
  }


  memset(&dataF, 0, sizeof(BackUpDataFolder));
  if (!eepromReadSlot(3, (uint8_t*)&dataF)) {
    Serial.println("[EEPROM] read slot backup MP3 folder ---- failed");
    ErrorB = true;
  }
  else
  {  radioSDFolder = String(dataF.SDFolder);
  //StandartWiFiPass[31] = '\0';
  //StandartWiFiPass = String(dataW.WiFi_Pass);
  }



  if (ErrorB)
  {
    Serial.printf("\n[EEPROM] read slot TOTAL backup --- FAILED:\n\nCurrent values:\n GPS: %s , %s\n WiFi Pass: %s\n MP3 folder:%s\n\n", weatherLat, weatherLon, StandartWiFiPass, radioSDFolder);
    return false;
  }
  else
  {
  Serial.printf("\n[EEPROM] read slot backup --- OK:\n GPS: %s , %s\n WiFi Pass: %s\n MP3 folder:%s\n", weatherLat, weatherLon, StandartWiFiPass, radioSDFolder);
    return true;
  }
}

bool SaveBackUpToEPPR()
{
BackUpDataGPS dataG;
BackUpDataWiFi dataW;
BackUpDataFolder dataF;

  // Очистка всей структуры перед использованием!
  memset(&dataG, 0, sizeof(BackUpDataGPS));

  strncpy(dataG.GPS_LAT, weatherLat.c_str(),  sizeof(dataG.GPS_LAT) - 1);
  dataG.GPS_LAT[sizeof(dataG.GPS_LAT)-1] = '\0';

  strncpy(dataG.GPS_LON, weatherLon.c_str(),  sizeof(dataG.GPS_LON) - 1);
  dataG.GPS_LON[sizeof(dataG.GPS_LON)-1] = '\0';

  if (weatherCelsius) dataG.icCels = 1; else dataG.icCels = 0;
  if (eepromWriteSlot(1, (uint8_t*)&dataG)) {
    Serial.printf("GPS saved to EEPROM: Lat: %s, Lon: %s, Celsius(1 - yes): %d\n", dataG.GPS_LAT ,dataG.GPS_LON, dataG.icCels);
  }
  else 
    return false;

  memset(&dataW, 0, sizeof(BackUpDataWiFi));
  strncpy(dataW.WiFi_Pass, StandartWiFiPass, sizeof(dataW.WiFi_Pass) - 1);
  dataW.WiFi_Pass[sizeof(dataW.WiFi_Pass)-1] = '\0';

  if (eepromWriteSlot(2, (uint8_t*)&dataW)) {
    Serial.printf("Wifi pass saved to EEPROM: %s\n", dataW.WiFi_Pass);
  }
  else 
    return false;

  memset(&dataF, 0, sizeof(BackUpDataFolder));
  strncpy(dataF.SDFolder, radioSDFolder.c_str(), sizeof(dataF.SDFolder) - 1);
  dataF.SDFolder[sizeof(dataF.SDFolder)-1] = '\0';

  if (eepromWriteSlot(3, (uint8_t*)&dataF)) {
    Serial.printf("MP3 folder saved to EEPROM: %s\n", dataF.SDFolder);
  }
  else 
    return false;

  return true;
}



// ======================= ОБРАБОТКА ТАЧА =======================
void handleTouch(uint16_t x, uint16_t y) {
  if (keyboardActive) {
    if (handleKeyboardTouch(x, y)) {
      delay(150);
      return;
    }
  }
  
  // Нижние вкладки
  if (y >= TAB_Y && y <= TAB_Y + TAB_H && !keyboardActive) {
    int newScreen = x / TAB_W;
    if (newScreen != currentScreen) {
      currentScreen = newScreen;
      
      if (lastScreen == 4 && currentScreen != lastScreen)
        ButtonScreen2 = 0;
      
      if (currentScreen == 1) {
        prevDay = 0;
        clockInitialized = false;
      }
      
      digitalWrite(LED_G, LOW);
      delay(30);
      digitalWrite(LED_G, HIGH);
      
      switch(currentScreen) {
        case 0: drawPipBoyScreen(); break;
        case 1: drawPipBoyScreen1(); break;
        case 2: drawPipBoyScreen2(); break;
        case 3: 
          //weatherForceUpdate(); 
          drawPipBoyScreen3(); 
          break;
        case 4: drawPipBoyScreen4(); break;
      }
      
      drawTabButtons();
    }

    
    delay(150);
    return;
  }
  
  // Обработка по экранам
  if (currentScreen == 0) {
    vaultFrame = (vaultFrame + 1) % 16;
    drawVaultBoy(bitmapStartX, 28, vaultFrame);
  }
  else if (currentScreen == 2) {
      handleRadioSetButtons(x, y);
  }
  else if (currentScreen == 4) {
        HandleButtonsScreen4(x, y);
  if (weatherSettingsActive) {
        handleWeatherSettingsTouch(x, y);
      } else if (radioSettingsActive) {
        handleRadioSettingsTouch(x, y);
      } else {    
        if (ButtonScreen2 == 1)
          handleWiFiTouch(x, y);
      }
  }
  
  delay(150);
}

// ======================= ОТРИСОВКА ЭКРАНОВ =======================


// Функция рисования стрелки ветра
void drawWindArrow(int x, int y, int degrees, int size, uint16_t color) {
  // Переводим в радианы (0° = Север = вверх)
  float angle = radians(degrees - 180); // -180 чтобы 0° смотрел вверх
  
  // Конец стрелки
  int x2 = x + (size/1.3) * sin(angle);
  int y2 = y - (size/1.3) * cos(angle);
  
  // Линия стрелки
  tft.drawLine(x, y, x2, y2, color);
  
  // Наконечник (треугольник)
  float arrowSize = size / 2.5;
  float angle1 = angle + radians(150);
  float angle2 = angle - radians(150);
  
  int xa1 = x2 + arrowSize * sin(angle1);
  int ya1 = y2 - arrowSize * cos(angle1);
  int xa2 = x2 + arrowSize * sin(angle2);
  int ya2 = y2 - arrowSize * cos(angle2);
  
  tft.fillTriangle(x2, y2, xa1, ya1, xa2, ya2, color);
}

void drawScanlines() {
  for (int16_t y = 4; y < tft.height(); y += 5) {
    tft.drawFastHLine(0, y, tft.width(), tft.color565(0, 35, 0));
  }
}

void drawScanlinesButtons(int16_t xS, int16_t yS, int16_t hS, int16_t wS) {
  yS = 4 + round((yS - 4) / 5.0) * 5;
  for (int16_t y = yS; y < (yS + hS); y += 5) {
    tft.drawFastHLine(xS, y, wS, tft.color565(0, 35, 0));
  }
}

void drawVaultBoy(int16_t cx, int16_t cy, int8_t frame) {
  if (frame > 16) frame = 0;
  
  tft.fillRect(cx, cy, 170, 175, TFT_BLACK);
  drawScanlinesButtons(cx, cy, 175, 170);
  tft.drawBitmap(cx, cy, specialBMP[frame], 170, 170, TFT_GREEN);
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.setCursor(bitmapStartX, 185);
  tft.print("SAM - Level ");
  tft.print(frame);
}

void drawPipBoyScreen() {
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  updateHPAP();
  // Top bar
  tft.drawRect(0, 0, 320, 28, TFT_GREEN);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setCursor(12, 6);
  tft.print("STATS");
  tft.setTextSize(1);
  tft.setCursor(80, 10);
  tft.print("LVL 20");
  tft.setCursor(135, 10);
  tft.printf("HP %d/%d\n", currentHP, hpMax);
  tft.setCursor(210, 10);
  tft.printf("AP %d/%d", currentAP, apMax);
  if (currentHP == hpMax)
  {
    tft.setCursor(280, 10);
    tft.print("XP MAX");
  }

  // Left panel
  tft.setTextSize(1);
  tft.setCursor(12, 38);
  tft.setTextColor(TFT_GREEN);
  if (WiFi.status() == WL_CONNECTED) 
  {
    tft.drawRect(9, 35, 29, 15, TFT_GREEN);
    tft.print("WiFi");
  }else
  {
    tft.print("WiFi");
  }
  tft.setCursor(12, 58);
  if (radioIsPlaying()) 
      {
        tft.drawRect(9, 55, 29, 15, TFT_GREEN);
        tft.print("RAD");
      }else
      {
        tft.print("RAD");
      }

  tft.setCursor(12, 78);
  tft.fillRect(9, 75, 35, 15, TFT_BLACK);
  drawScanlinesButtons(9, 75, 18, 35);
  CurrentWeather* w = weatherGetCurrent();
      String src = (w->source == WEATHER_PRIMARY) ? "[W]" : 
               (w->source == WEATHER_OPENMETEO) ? "[O]" : "[E]";
      //tft.drawString(src, 310, 35);
      if ((src == "[W]") || (src == "[O]"))
      {
        src = "W " + src;
        tft.drawRect(9, 75, 35, 15, TFT_GREEN);
        tft.print(src);
      }
      else
      {
        if (eepromFound)
          tft.print("W [E]");
        else
        {
           if (!weatherHasData()) 
            tft.print("ERR W");
          else
            tft.print("ERR E");
        }
          
      }
  
  // Vault Boy
  drawVaultBoy(bitmapStartX, 28, vaultFrame);
  
  // Right panel
   tft.setTextSize(1);
  tft.setCursor(240, 45);
  tft.print("(12) Stimpak");
  tft.setCursor(240, 68);
  tft.print("Limbs");
  
  lastScreen = currentScreen;
}

// ======================= ЭКРАН 1: CLOCK =======================
void SetupDigits() {
  tft.fillScreen(clockBackgroundColor);
  //tft.fillScreen(TFT_BLACK);
  drawScanlines();
  

  for (size_t i = 0; i < 6; i++) {
    digs[i] = new Digit(0);
    digs[i]->Height(tft.fontHeight());
  }
   //Debug("textWidth 1", tft.textWidth("1"));
   //Debug("textWidth :", tft.textWidth(":"));
   //Debug("textWidth 8", tft.textWidth("8"));
   //Debug("fontHeight", tft.fontHeight());

  CalculateDigitOffsets();
  clockInitialized = true;
}

void CalculateDigitOffsets() {
  tft.setTextFont(clockFont);
  tft.setTextSize(clockSize);
  tft.setTextDatum(clockDatum);
  int y = timeY;
  int width = 320;//tft.width();
  int DigitWidth = tft.textWidth("8");
  int colonWidth = tft.textWidth(":");
  int left = SHOW_AMPM ? 10 : (width - DigitWidth * 6 - colonWidth * 2) / 2;
  digs[0]->SetXY(left, y);                      // HH
  digs[1]->SetXY(digs[0]->X() + DigitWidth, y); // HH

  colons[0] = digs[1]->X() + DigitWidth; // :

  digs[2]->SetXY(colons[0] + colonWidth, y); // MM
  digs[3]->SetXY(digs[2]->X() + DigitWidth, y);

  colons[1] = digs[3]->X() + DigitWidth; // :

  digs[4]->SetXY(colons[1] + colonWidth, y); // SS
  digs[5]->SetXY(digs[4]->X() + DigitWidth, y);

  ampm[0] = digs[5]->X() + DigitWidth + 4;
  ampm[1] = y - 2;
}

void DrawColons() {
  tft.setTextFont(clockFont);
  tft.setTextSize(clockSize);
  tft.setTextDatum(clockDatum);
  tft.drawChar(':', colons[0], timeY);
  tft.drawChar(':', colons[1], timeY);
}

void DrawAmPm() {
  if (SHOW_AMPM) {
    tft.setTextSize(3);
    tft.drawChar(ispm ? 'P' : 'A', ampm[0], ampm[1]);
    tft.drawChar('M', ampm[0], ampm[1] + tft.fontHeight());
  }
}

void DrawDigitsAtOnce() {
  //tft.setTextDatum(TL_DATUM);
  for (size_t f = 0; f <= digs[0]->Height(); f++) // For all animation frames...
  {
    for (size_t di = 0; di < 6; di++) // for all Digits...
    {
      Digit *dig = digs[di];
      if (dig->Value() == dig->NewValue()) // If Digit is not changing...
      {
        if (f == 0) //... and this is first frame, just draw it to screeen without animation.
        { 
          fillLinesSprite(dig);
          sprite.drawNumber(dig->Value(), 0, 0);
          sprite.pushSprite(dig->X(), dig->Y());
        }
      }
      else // However, if a Digit is changing value, we need to draw animation frame "f"
      {
        dig->Frame(f);                                                       // Set the animation offset
  
        fillLinesSprite(dig);
        sprite.drawNumber(dig->Value(), 0, -dig->Frame());
        sprite.pushSprite(dig->X(), dig->Y());  
        delay(5);                    // Scroll up the current value
        fillLinesSprite(dig);
        sprite.drawNumber(dig->NewValue(), 0, dig->Height() - dig->Frame()); // while make new value appear from below
        sprite.pushSprite(dig->X(), dig->Y());                               // Draw the current animation frame to actual screen.
      }
    }
    delay(5);
  }

  // Once all animations are done, then we can update all Digits to current new values.
  for (size_t di = 0; di < 6; di++)
  {
    Digit *dig = digs[di];
    dig->Value(dig->NewValue());
  }
}


void DrawADigit(Digit *digg)
{
  if (digg->Value() == digg->NewValue())
  {
    fillLinesSprite(digg);
    sprite.drawNumber(digg->Value(), 0, 0);
    sprite.pushSprite(digg->X(), digg->Y());
  }
  else
  {
    
    for (size_t f = 0; f <= digg->Height(); f++)
    {
      digg->Frame(f);
      fillLinesSprite(digg);
      sprite.drawNumber(digg->Value(), 0, -digg->Frame());
      sprite.pushSprite(digg->X(), digg->Y());
      delay(5);
      fillLinesSprite(digg);
      sprite.drawNumber(digg->NewValue(), 0, digg->Height() - digg->Frame());
      sprite.pushSprite(digg->X(), digg->Y());
      delay(5);
    }
    digg->Value(digg->NewValue());
  }
}

void fillLinesSprite(Digit *digg)
{
//tft.fillRect(digg->X(), digg->Y(), sprite.width(),sprite.height(), clockBackgroundColor);
      sprite.fillSprite(clockBackgroundColor);
          int yS = 4 + round((timeY - 4) / 5.0) * 5 - timeY;
          for (int16_t y = yS; y < (yS + sprite.height()); y += 5) 
          {
            sprite.drawFastHLine(0, y, sprite.width(), tft.color565(0, 35, 0));
          }

}

void DrawDigitsOneByOne()
{
  tft.setTextDatum(TL_DATUM);
  for (size_t i = 0; i < 6; i++)
  {
    DrawADigit(digs[5 - i]);
  }
}


void ParseDigits(time_t utc) {
  time_t local = myTZ.toLocal(utc, &tcr);
  digs[0]->NewValue(hour(local) / 10);
  digs[1]->NewValue(hour(local) % 10);
  digs[2]->NewValue(minute(local) / 10);
  digs[3]->NewValue(minute(local) % 10);
  digs[4]->NewValue(second(local) / 10);
  digs[5]->NewValue(second(local) % 10);
  ispm = isPM(local);
}

void DrawDate(time_t utc) {
  time_t local = myTZ.toLocal(utc, &tcr);
  int dd = day(local);
  int mth = month(local);
  int yr = year(local);

  static int prevDayCache = 0;
  if (dd != prevDayCache || lastScreen != currentScreen) {
    tft.setTextDatum(BC_DATUM);
    char buffer[50];
    if (NOT_US_DATE) {
      sprintf(buffer, "%02d.%02d.%d", dd, mth, yr);
    } else {
      sprintf(buffer, "%02d/%02d/%d", mth, dd, yr);
    }

    tft.setTextSize(4);
    int h = tft.fontHeight();
    tft.fillRect(0, 170 - h, 320, h, TFT_BLACK);
    drawScanlinesButtons(0, 170 - h, h, 320);
    tft.drawString(buffer, 160, 170);

    int dow = weekday(local);
    String dayNames[] = {"", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    tft.setTextSize(3);
    tft.fillRect(0, 130 - h, 320, h, TFT_BLACK);

    drawScanlinesButtons(0, 130 - h, h, 320);
    tft.drawString(dayNames[dow], 160, 130);
    prevDayCache = dd;
  }
}

void drawPipBoyScreen1() {
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("CLOCK", 160, 10);
  

  if (!clockInitialized) {
   SetupDigits();
   }
  drawTabButtons();
  if (WiFi.status() == WL_CONNECTED) {
    rtcSyncNtpIfNeeded();
  }
  
  if (timeStatus() == timeNotSet) {
     tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("CLOCK", 160, 10);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    
  tft.fillRect(40, 80, 240, 80, TFT_BLACK);
  tft.drawRect(40, 80, 240, 80, TFT_GREEN);
    //tft.setTextColor(tft.color565(0, 50, 0));

    tft.setTextColor(TFT_RED);
    tft.drawString("NO TIME SYNC", 160, 110);
    tft.drawString("Connect WiFi", 160, 130);
    lastScreen = currentScreen;
    return;
  }
  
  time_t current = now();
  prevDisplay = current;
  //DrawDate();
  
  DrawDate(prevDisplay);
  DrawColons();
  ParseDigits(prevDisplay);
  DrawDigitsAtOnce();
  DrawAmPm();
  
  lastScreen = currentScreen;
}


void drawPipBoyScreen2() {
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
 tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("RADIO", 160, 10);

  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(TC_DATUM);
  
  //tft.setCursor(5, 25);
  tft.drawRect(6, 24, 30, 14, TFT_GREEN);
  String src = (radioPlaySource == 0) ? "SD" : (radioPlaySource == 1) ? "WiFi" : "Ext";
  //tft.print("Play: ");
  tft.drawString(src, 6 + 30 / 2, 20 + 14 / 2);

  //String VolumeInfo = "Volume: " + String(radioGetVolume());
 
  //tft.drawString(VolumeInfo, 160, 148);

  if (WiFi.status() != WL_CONNECTED && radioPlaySource == 1) 
  {
    if (radioIsPlaying())
      UpdateMetaData();
      //tft.drawString(radioGetStationName(), 160, 100);
    else
      tft.drawString("ConnectWiFi", 160, 100);
      /*
      dacWrite(26, 128);
      delay(1000);
      dacWrite(26, 200);
      delay(2000);
      dacWrite(26, 128);
      */
      //delay(1000);
  }
  else
  {
    //if (radioIsPlaying())
      UpdateMetaData();
    //else
      //radioPlayStation(RadStationInd);
   // tft.drawString(radioGetStationName(), 160, 100);
    
  }

      //UpdateMetaData();

  drawRadioSetButtons();
  drawTabButtons();
  lastScreen = currentScreen;
}

void drawPipBoyScreen3() {
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("WEATHER", 160, 10);
  
  CurrentWeather* w = weatherGetCurrent();
  
  if (WiFi.status() != WL_CONNECTED) 
  {
    weatherLoadFromEEPROM();
  }

  if (!weatherHasData()) {
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);

    tft.fillRect(40, 80, 240, 80, TFT_BLACK);
    tft.drawRect(40, 80, 240, 80, TFT_GREEN);
    tft.drawString("No weather data", 160, 110);
    if (WiFi.status() != WL_CONNECTED) 
      tft.drawString("Connect WiFi", 160, 130);
    else
    {
      tft.setTextColor(TFT_GREEN);
      tft.drawString("Loading ...", 160, 120);
      weatherForceUpdate();
    }
    needUpdateScreenWeather = true;
    weatherUpdate();
    drawTabButtons();
    lastScreen = currentScreen;
    return;
  }
  
  drawTabButtons();
  needUpdateScreenWeather = false;
  // Температура со знаком
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(6);
  
  int tempVal = w->temperature.toInt();
  String unit = weatherCelsius ? "C" : "F";
  unit =  " " + unit; // "\xB0"
  String tempDisplay;
  
  if (tempVal > 0) {
    tempDisplay = "+" + w->temperature + unit;
  } else if (tempVal < 0) {
    tempDisplay = "-" + w->temperature + unit;
  } else {
    tempDisplay = "0" + unit;
  }
  
  tft.drawString(tempDisplay, 160, 40);
  int Widthstr = tft.textWidth(tempDisplay);

    Serial.printf("Text width temp: %d\n", Widthstr);
  tft.drawCircle((320/2) + (Widthstr/2) - 50 , 45, 6, TFT_GREEN);
  tft.drawCircle((320/2) + (Widthstr/2) - 50, 45, 5, TFT_GREEN);
  tft.drawCircle((320/2) + (Widthstr/2) - 50, 45, 4, TFT_GREEN);
  
  // Условия
  //tft.setTextSize(2);
  //tft.drawString(w->condition, 160, 110);
  
  drawWeatherIconCentered(160, 110, w->condition, TFT_GREEN);
  // Ветер со стрелкой
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.setCursor(60, 150);
  tft.print("Wind: ");
  // Использование вместо текста:
  drawWindArrow(110, 153, w->windDir, 20, TFT_WHITE);

  tft.setCursor(130, 150);
  //tft.print(w->windDir);
  //tft.print("   ");
  tft.print(w->wind);
  tft.print(" km/h");
  
  // Координаты
  tft.setCursor(60, 170);
  tft.print("Coords: ");
  tft.print(weatherLat.substring(0, 6));
  tft.print(", ");
  tft.print(weatherLon.substring(0, 6));
  
  // Время обновления (placeholder, обновляется отдельно)
  tft.setCursor(60, 190);
  tft.print("Updated: ");
  updateWeatherTimeDisplay();
  weatherUpdate();
  // Индикатор источника
  tft.setTextSize(1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(tft.color565(0, 100, 0));
  String src = (w->source == WEATHER_PRIMARY) ? "[W]" : 
               (w->source == WEATHER_OPENMETEO) ? "[O]" : "[E]";
  tft.drawString(src, 310, 35);
  //drawTabButtons();
  lastScreen = currentScreen;
}

void updateWeatherTimeDisplay() {
  CurrentWeather* w = weatherGetCurrent();
  if (!weatherHasData()) return;
  
  int mins = weatherGetAgeMinutes();
  if (mins < 0) return;
  
  bool isOld = (mins > 60);
  if (mins > 60) 
  {
    mins = 60;
  }
  
  // Координаты поля времени
  int timeX = 60 + 54;  // После "Updated: "
  int timeY = 190;
  int timeW = 100;
  int timeH = 12;
  
  // Закрасить старое значение
  tft.fillRect(timeX, timeY - 2, timeW, timeH, TFT_BLACK); 
  drawScanlinesButtons(timeX, timeY - 2, timeH, timeW);
  // Новое значение
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(isOld ? TFT_ORANGE : TFT_GREEN);
  tft.setCursor(timeX, timeY);
  
  if (WiFi.status() == WL_CONNECTED) {
    tft.print(mins);
    tft.print(" min ago");
  } else {
    tft.print(mins);
    tft.print(" min (WiFi OFF)");
  }
  
  // Индикатор источника
  tft.setTextSize(1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(tft.color565(0, 100, 0));
  // Закрасить старое значение
  tft.fillRect(280, 20, 40, 20, TFT_BLACK); 
  drawScanlinesButtons(280, 20, 24, 40);
  String src = (w->source == WEATHER_PRIMARY) ? "[W]" : 
               (w->source == WEATHER_OPENMETEO) ? "[O]" : "[E]";
  tft.drawString(src, 310, 35);
  // Предупреждение о старых данных
  //if (isOld) {
  //  tft.setTextColor(TFT_RED);
  //  tft.setCursor(60, 215);
   // tft.print("Data expired!     ");
  //}
}


// ======================= ЭКРАН 4: GENERAL =======================
void drawPipBoyScreen4() {
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  drawButtonsScreen4();
  lastScreen = currentScreen;
}

void drawButtonsScreen4() {
  int16_t x = 3, yS = 0, y = 0, tab_b_W = 70, tab_b_H = 30; 
  int active = ButtonScreen2;
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  int ButtYW = (tab_b_H + yS + 8);
  
  for (int i = 1; i <= 4; i += 1) {
    y = ButtYW * i;
    if (i != active) {
      tft.fillRect(x, y, tab_b_W, tab_b_H, TFT_BLACK);
      drawScanlinesButtons(x, y, tab_b_H, tab_b_W);
      tft.drawRect(x, y, tab_b_W, tab_b_H, TFT_GREEN);
      tft.setTextColor(TFT_GREEN);
    } else {
      tft.fillRect(x, y, tab_b_W, tab_b_H, TFT_GREEN);
      tft.setTextColor(TFT_BLACK);
    }
  
    switch(i) {
      case 1:
        tft.drawString("Wi-Fi", x + tab_b_W/2, y + tab_b_H/2);
        break;
      case 2:
        tft.drawString("RADIO", x + tab_b_W/2, y + tab_b_H/2);
        break;
      case 3:
        tft.drawString("TIME", x + tab_b_W/2, y + tab_b_H/2);
        break;
      case 4:
        tft.drawString("W.SET", x + tab_b_W/2, y + tab_b_H/2);
        break;
    }
  }  
    
  drawTabButtons();
}

void HandleButtonsScreen4(uint16_t xTouch, uint16_t yTouch) {
  int16_t x = 3, yS = 0, y = 0, tab_b_W = 70, tab_b_H = 30; 
  int active = 0;
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  int ButtYW = (tab_b_H + yS + 8 );
  
  if ((yTouch >= y && yTouch <= ButtYW * 5) && (xTouch >= x && xTouch <= x + tab_b_W)) {
    active = yTouch / ButtYW;
    if (active > 4) active = 4;
    if (active < 1) active = 1;
    if (active != ButtonScreen2) {
      tft.fillScreen(TFT_BLACK);
      drawScanlines();
    }
    ButtonScreen2 = active;
    
    for (int i = 1; i <= 4; i += 1) {
      y = ButtYW * i;
      if (i != active) {
        tft.fillRect(x, y, tab_b_W, tab_b_H, TFT_BLACK);
        drawScanlinesButtons(x, y, tab_b_H, tab_b_W);
        tft.drawRect(x, y, tab_b_W, tab_b_H, TFT_GREEN);
        tft.setTextColor(TFT_GREEN);
      } else {
        tft.fillRect(x, y, tab_b_W, tab_b_H, TFT_GREEN);
        tft.setTextColor(TFT_BLACK);
      }
    
      switch(i) {
        case 1:
          tft.drawString("Wi-Fi", x + tab_b_W/2, y + tab_b_H/2);
          break;
        case 2:
          tft.drawString("RADIO", x + tab_b_W/2, y + tab_b_H/2);
          break;
        case 3:
          tft.drawString("TIME", x + tab_b_W/2, y + tab_b_H/2);
          break;
        case 4:
          tft.drawString("W.SET", x + tab_b_W/2, y + tab_b_H/2);
          break;
      }
    }  
    
    digitalWrite(LED_G, LOW);
    delay(30);
    digitalWrite(LED_G, HIGH);
    
    switch(active) {
      case 1:
        weatherSettingsActive = false;
        radioSettingsActive = false;
        drawWiFiScreen();
        break;
      case 2:
        weatherSettingsActive = false;
        radioSettingsActive = true;
        drawRadioSettings();
        break;
      case 3:
        weatherSettingsActive = false;
        radioSettingsActive = false;
        break; 
      case 4:
        weatherSettingsActive = true;
        radioSettingsActive = false;
        drawWeatherSettings();
        break;
    }
    
    drawTabButtons();
  }
}

void drawWeatherSettings() {
  weatherSettingsActive = true;
  
  tft.fillRect(LIST_X, 10, 320 - LIST_X - 5, 186, TFT_BLACK);
  tft.drawRect(LIST_X, 10, 320 - LIST_X - 5, 186, TFT_GREEN);
  //tft.drawRect(LIST_X + 2, LIST_Y + 2, LIST_W - 4, LIST_H - 4, tft.color565(0, 100, 0));
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("WEATHER SETTINGS", LIST_X + (320 - LIST_X - 5)/2, 15);
  
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  
  tft.setCursor(LIST_X + 10, 45);
  tft.print("GPS LATITUDE:");
  tft.fillRect(LIST_X + 95, 40, 110, 20, TFT_BLACK);
  tft.drawRect(LIST_X + 95, 40, 110, 20, TFT_GREEN);
  tft.setCursor(LIST_X + 100, 47);
  tft.print(weatherLat);
  
  tft.setCursor(LIST_X + 10, 65);
  tft.print("GPS LONGITUDE:");
  tft.fillRect(LIST_X + 95, 65, 110, 20, TFT_BLACK);
  tft.drawRect(LIST_X + 95, 65, 110, 20, TFT_GREEN);
  tft.setCursor(LIST_X + 100, 72);
  tft.print(weatherLon);
  
  tft.setCursor(LIST_X + 10, 105);
  tft.print("UNITS:");
  
  int btnX = LIST_X + 80;
  int btnY = 100;
  int btnW = 60;
  int btnH = 25;
  
  if (weatherCelsius) {
    tft.fillRect(btnX, btnY, btnW, btnH, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(btnX, btnY, btnW, btnH, TFT_BLACK);
    tft.drawRect(btnX, btnY, btnW, btnH, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  tft.setTextDatum(MC_DATUM);
  tft.drawString("C", btnX + btnW/2, btnY + btnH/2);
  
  btnX = LIST_X + 150;
  if (!weatherCelsius) {
    tft.fillRect(btnX, btnY, btnW, btnH, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(btnX, btnY, btnW, btnH, TFT_BLACK);
    tft.drawRect(btnX, btnY, btnW, btnH, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  tft.drawString("F", btnX + btnW/2, btnY + btnH/2);
  
  tft.fillRect(LIST_X + (320 - LIST_X - 5)/2 - 65, 150, 60, 25, TFT_BLACK);
  tft.drawRect(LIST_X + (320 - LIST_X - 5)/2 - 65, 150, 60, 25, TFT_GREEN);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("SAVE", LIST_X + ((320 - LIST_X - 5)/2 - 70/2), 163);
  
  tft.fillRect(LIST_X + (320 - LIST_X - 5)/2 + 5, 150, 60, 25, TFT_BLACK);
  tft.drawRect(LIST_X + (320 - LIST_X - 5)/2 + 5, 150, 60, 25, TFT_GREEN);
  tft.drawString("CANCEL", LIST_X + ((320 - LIST_X - 5)/2 + 70/2), 163);
  
  tft.setTextColor(tft.color565(0, 100, 0));
  tft.setTextDatum(TC_DATUM);
  //tft.drawString("Tap fields to edit", LIST_X + (320 - LIST_X - 5)/2 , 178);
}

void handleWeatherSettingsTouch(uint16_t x, uint16_t y) {
  if (x < LIST_X || x > LIST_X + LIST_W || y < LIST_Y || y > LIST_Y + LIST_H) {
    return;
  }
  
  if (x >= LIST_X + 100 && x <= LIST_X + 215 && y >= 40 && y <= 60) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    editlat = true;
    showKeyboard(weatherLat.c_str());
    
   // weatherLat = String(getKeyboardInput());
    //drawWeatherSettings();
    return;
  }
  
  if (x >= LIST_X + 100 && x <= LIST_X + 215 && y >= 65 && y <= 85) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    editlon = true;
    showKeyboard(weatherLon.c_str());
    //weatherLon = String(getKeyboardInput());
    //drawWeatherSettings();
    return;
  }
  
//celsus button
  if (x >= LIST_X + 80 && x <= LIST_X + 140 && y >= 100 && y <= 125) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    weatherCelsius = true;
    drawWeatherSettings();
    return;
  }
  // farengheit button
  if (x >= LIST_X + 150 && x <= LIST_X + 210 && y >= 100 && y <= 125) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    weatherCelsius = false;
    drawWeatherSettings();
    return;
  }
  
  // OK
  if (x >= LIST_X + (320 - LIST_X - 5)/2 - 65 && x <= LIST_X + (320 - LIST_X - 5)/2 - 5 && y >= 150 && y <= 175) {
    digitalWrite(LED_G, LOW);
    delay(30);
    digitalWrite(LED_G, HIGH);
    weatherSettingsActive = false;
    SaveBackUpToEPPR(); 
    weatherForceUpdate();
    drawPipBoyScreen4();
    return;
  }
  
  // CANCEL
  if (x >= LIST_X + (320 - LIST_X - 5)/2 + 5 && x <= LIST_X + (320 - LIST_X - 5)/2 + 65 && y >= 150 && y <= 175) {
    digitalWrite(LED_R, LOW);
    delay(30);
    digitalWrite(LED_R, HIGH);
    weatherSettingsActive = false;
    drawPipBoyScreen4();
    return;
  }
}

// ======================= ВКЛАДКИ =======================



// Проверка корректности пути (формат)
bool isValidPath(const String& path) {
  if (path.length() < 2) return false;
  if (!path.startsWith("/")) return false;
  if (path.indexOf("..") != -1) return false;           // запрещаем поднятие вверх
  if (path.indexOf("//") != -1) return false;           // двойной слеш
  if (path.length() > 64) return false;                 // слишком длинный путь
  
  // Разрешены только буквы, цифры, _, -, ., / и пробел
  for (char c : path) {
    if (!isalnum(c) && c != '/' && c != '_' && c != '-' && c != '.' && c != ' ') {
      return false;
    }
  }
  return true;
}




bool checkSDPath(const char* path) {
  if (!sdCardInitialized) {
    Serial.println("[checkSDPath] SD not initialized");
    return false;
  }
  if (!path || strlen(path) < 2) {
    Serial.println("[checkSDPath] Path too short");
    return false;
  }

  String p = String(path);
  
  // Сначала проверяем формат пути
  if (!isValidPath(p)) {
    Serial.println("[checkSDPath] Invalid path format");
    return false;
  }

  // Защита от повторного вызова
  static bool checking = false;
  if (checking) return false;
  checking = true;

  Serial.printf("[checkSDPath] Testing: '%s'\n", path);

  //digitalWrite(5, LOW);
 // delay(10);

  File dir = SD.open(path);
  bool ok = (dir && dir.isDirectory());
  if (dir) dir.close();

  //digitalWrite(5, HIGH);
  checking = false;

  Serial.printf("[checkSDPath] Result: %s\n", ok ? "VALID DIRECTORY" : "NOT FOUND / NOT DIRECTORY");
  return ok;
}




void drawRadioSettings() {
  radioSettingsActive = true;
  tft.fillRect(LIST_X, 10, 320 - LIST_X - 5, 186, TFT_BLACK);
  tft.drawRect(LIST_X, 10, 320 - LIST_X - 5, 186, TFT_GREEN);

  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("RADIO SETTINGS", LIST_X + (320 - LIST_X - 5)/2, 15);

  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  //tft.setCursor(LIST_X + 10, 45);
  tft.drawString("Select mp3 radio:", (320 -LIST_X) / 2, 40 );

  int btnW = 60, btnH = 25, btnY = 60;
  int startX = LIST_X + 12;

  // SD
  if (radioPlaySource == 0) {
    tft.fillRect(startX, btnY, btnW, btnH, TFT_GREEN); tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(startX, btnY, btnW, btnH, TFT_BLACK);
    tft.drawRect(startX, btnY, btnW, btnH, TFT_GREEN); tft.setTextColor(TFT_GREEN);
  }
  tft.setTextDatum(MC_DATUM);
  tft.drawString("SD", startX + btnW/2, btnY + btnH/2);

  startX += btnW + 10;
  // WiFi
  if (radioPlaySource == 1) {
    tft.fillRect(startX, btnY, btnW, btnH, TFT_GREEN); tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(startX, btnY, btnW, btnH, TFT_BLACK);
    tft.drawRect(startX, btnY, btnW, btnH, TFT_GREEN); tft.setTextColor(TFT_GREEN);
  }
  tft.drawString("WiFi", startX + btnW/2, btnY + btnH/2);

  startX += btnW + 10;
  // Ext
  if (radioPlaySource == 2) {
    tft.fillRect(startX, btnY, btnW, btnH, TFT_GREEN); tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(startX, btnY, btnW, btnH, TFT_BLACK);
    tft.drawRect(startX, btnY, btnW, btnH, TFT_GREEN); tft.setTextColor(TFT_GREEN);
  }
  tft.drawString("Ext", startX + btnW/2, btnY + btnH/2);

  // Поле папки (только если SD)
  if (radioPlaySource == 0) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_GREEN);
    if (!sdCardInitialized)
    {
      tft.drawString("Insert SD card", LIST_X + (320 - LIST_X - 5)/2, 120);
    } else
    {
      tft.setCursor(LIST_X + 10, 110);
      tft.print("Folder:");
      int fieldX = LIST_X + 70, fieldY = 105, fieldW = 150, fieldH = 20;
      tft.fillRect(fieldX, fieldY, fieldW, fieldH, TFT_BLACK);
      tft.drawRect(fieldX, fieldY, fieldW, fieldH, TFT_GREEN);
      tft.setCursor(fieldX + 5, fieldY + 5);
      bool pathOk = checkSDPath(radioSDFolder.c_str());
      tft.setTextColor(pathOk ? TFT_GREEN : TFT_RED);
      tft.print(radioSDFolder);
    }

  }

  // SAVE / CANCEL
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN);
  tft.fillRect(LIST_X + (320 - LIST_X - 5)/2 - 65, 150, 60, 25, TFT_BLACK);
  tft.drawRect(LIST_X + (320 - LIST_X - 5)/2 - 65, 150, 60, 25, TFT_GREEN);
  tft.drawString("SAVE", LIST_X + ((320 - LIST_X - 5)/2 - 35), 163);

  tft.fillRect(LIST_X + (320 - LIST_X - 5)/2 + 5, 150, 60, 25, TFT_BLACK);
  tft.drawRect(LIST_X + (320 - LIST_X - 5)/2 + 5, 150, 60, 25, TFT_GREEN);
  tft.drawString("CANCEL", LIST_X + ((320 - LIST_X - 5)/2 + 35), 163);
}

void handleRadioSettingsTouch(uint16_t x, uint16_t y) {
  if (x < LIST_X || x > LIST_X + LIST_W || y < LIST_Y || y > LIST_Y + LIST_H) return;

  int btnY = 60, btnH = 25, btnW = 60;
  int startX = LIST_X + 12;

  // SD button
  if (y >= btnY && y <= btnY + btnH && x >= startX && x <= startX + btnW) {
    if (radioIsPlaying()) radioPause(); 
    digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
    radioPlaySource = 0; drawRadioSettings(); return;
  }
  startX += btnW + 10;

  // WiFi button
  if (y >= btnY && y <= btnY + btnH && x >= startX && x <= startX + btnW) {
    if (radioIsPlaying()) radioPause(); 
    digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
    radioPlaySource = 1; drawRadioSettings(); return;
  }
  startX += btnW + 10;

  // Ext button
  if (y >= btnY && y <= btnY + btnH && x >= startX && x <= startX + btnW) {
    if (radioIsPlaying()) radioPause();
    digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
    radioPlaySource = 2; drawRadioSettings(); return;
  }

  // Поле Folder (только SD)
  if (radioPlaySource == 0 && x >= LIST_X + 70 && x <= LIST_X + 220 && y >= 105 && y <= 125) {
    digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
    showKeyboard(radioSDFolder.c_str());
    return;
  }

  // SAVE
  if (x >= LIST_X + (320 - LIST_X - 5)/2 - 65 && x <= LIST_X + (320 - LIST_X - 5)/2 - 5 && y >= 150 && y <= 175) {
    digitalWrite(LED_G, LOW); delay(30); digitalWrite(LED_G, HIGH);
    radioSettingsActive = false;
    if (radioPlaySource == 0 && checkSDPath(radioSDFolder.c_str())) {
      SaveBackUpToEPPR();
      //loadSDPlaylist();   // вызов из radio_module
    }
    drawPipBoyScreen4();
    return;
  }

  // CANCEL
  if (x >= LIST_X + (320 - LIST_X - 5)/2 + 5 && x <= LIST_X + (320 - LIST_X - 5)/2 + 65 && y >= 150 && y <= 175) {
    digitalWrite(LED_R, LOW); delay(30); digitalWrite(LED_R, HIGH);
    radioSettingsActive = false;
    drawPipBoyScreen4();
    return;
  }
}

void handleRadioSetButtons(uint16_t x, uint16_t y) {
  int xp = TAB_W - 12;
  int yp = TAB_Y - 40;
  
  if (y < yp || y > yp + TAB_H) {
    return;
  }
    
    tft.setTextColor(TFT_GREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

  bool indicateRGB = false;
    for (int i = 0; i < 6; i++) {
      if (WiFi.status() != WL_CONNECTED && radioPlaySource == 1)
      {
        if (i < 1) 
        {
          i = 1; 
          indicateRGB = false; 
        }
      } else
      indicateRGB = true; 
      
      if (x >= xp * i + 5 && x <= xp * i + 5 + xp) 
      {
        switch(i) {
        case 0: {
                  //tft.fillRect(20, 80, 300, 40, TFT_BLACK);
                  //drawScanlinesButtons(20, 78, 82, 300);
                  //tft.drawString(radioGetStationName(), 160, 100);
                  
                  if (radioPlaySource == 0 && !sdPlaylistLoaded)
                  {
                    radioScanSDFolder();
                    if (sdPlaylistLoaded) radioPlay(); //radioPlaySDFolder(radioSDFolder); // играть с SD
                  }
                  else
                  {
                    if (!radioIsPlaying()) radioPlay();
                  }
                  //loadSDPlaylist();   // вызов из radio_module
                  
                    
                  ////if (radioPlaySource == 1)
                  //radioPlayStation(RadStationInd); 

                  radioSetVolume(RadVolume); 
                  //radioPlay(); 
                  break;
                }
        case 1: {
                  radioPause(); 
                  digitalWrite(LED_R, LOW);
                  delay(30);
                  digitalWrite(LED_R, HIGH);
                  break;
                }
        case 2: {
                  indicateRGB = false; 
                  if (radioPlaySource == 0)
                    if (radioPrevTrack()) indicateRGB = true; 
                  if (radioPlaySource == 1)
                    if (radioPrevStation()) indicateRGB = true; 
                  //tft.fillRect(20, 80, 300, 40, TFT_BLACK);
                  //drawScanlinesButtons(20, 78, 82, 300);
                  //tft.drawString(radioGetStationName(), 160, 100);         
                  RadStationInd = radioGetStationIndex();  
                  break;
                }
        case 3: {
                  indicateRGB = false; 
                  if (radioPlaySource == 0)
                    if (radioNextTrack()) indicateRGB = true;
                  if (radioPlaySource == 1)
                    if (radioNextStation()) indicateRGB = true; 
                  //tft.fillRect(20, 80, 300, 40, TFT_BLACK);
                  //drawScanlinesButtons(20, 78, 82, 300);
                  //tft.drawString(radioGetStationName(), 160, 100);  
                  RadStationInd = radioGetStationIndex(); 
                  break;
                }
        case 4: {
                  RadVolume -= 10;
                  if (RadVolume < 10) RadVolume = 10;
                  radioSetVolume(RadVolume); 
                  break;
                }
        case 5: {
                  RadVolume += 10;
                  if (RadVolume > 100) RadVolume = 100;
                  radioSetVolume(RadVolume);  
                  
                  break;
                }
        }
      }
    }
    UpdateMetaData();
    if (indicateRGB)
    {
      //drawPipBoyScreen2();
      digitalWrite(LED_B, LOW);
      delay(30);
      digitalWrite(LED_B, HIGH);
    }

    
}


void drawRadioSetButtons() {
    tft.setTextColor(TFT_GREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    int x = TAB_W - 12;
    int y = TAB_Y - 40;
    for (int i = 0; i < 6; i++) {
      tft.fillRect(x * i + 5, y, x, TAB_H, TFT_BLACK);
      drawScanlinesButtons(x * i + 5, y, TAB_H, TAB_W);
      tft.drawRect(x * i + 5, y, x, TAB_H, TFT_GREEN);
      switch(i) {
        case 0: tft.drawString("Play", x * i + 5 + x/2, y + TAB_H/2); break;
        case 1: tft.drawString("Pause", x * i + 5 + x/2, y + TAB_H/2); break;
        case 2: tft.drawString("Prev", x * i + 5 + x/2, y + TAB_H/2); break;
        case 3: tft.drawString("Next", x * i + 5 + x/2, y + TAB_H/2); break;
        case 4: tft.drawString("Vol -", x * i + 5 + x/2, y + TAB_H/2); break;
        case 5: tft.drawString("Vol +", x * i + 5 + x/2, y + TAB_H/2); break;
      }
      
    }
}

void drawTabButtons() {
  drawStatusButton(currentScreen == 0);
  drawSpecialButton(currentScreen == 1);
  drawSkillsButton(currentScreen == 2);
  drawWeatherButton(currentScreen == 3);
  drawGeneralButton(currentScreen == 4);
}

void drawStatusButton(bool active) {
  int x = 0;
  if (active) {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_BLACK);
    drawScanlinesButtons(x, TAB_Y, TAB_H, TAB_W);
    tft.drawRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.drawString("STATUS", x + TAB_W/2, TAB_Y + TAB_H/2);
}

void drawSpecialButton(bool active) {
  int x = TAB_W;
  if (active) {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_BLACK);
    drawScanlinesButtons(x, TAB_Y, TAB_H, TAB_W);
    tft.drawRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  tft.setTextDatum(MC_DATUM);
  tft.drawString("CLOCK", x + TAB_W/2, TAB_Y + TAB_H/2);
}

void drawSkillsButton(bool active) {
  int x = 2 * TAB_W;
  if (active) {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_BLACK);
    drawScanlinesButtons(x, TAB_Y, TAB_H, TAB_W);
    tft.drawRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  tft.setTextDatum(MC_DATUM);
  tft.drawString("RADIO", x + TAB_W/2, TAB_Y + TAB_H/2);
}

void drawWeatherButton(bool active) {
  int x = 3 * TAB_W;
  if (active) {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_BLACK);
    drawScanlinesButtons(x, TAB_Y, TAB_H, TAB_W);
    tft.drawRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WEATHER", x + TAB_W/2, TAB_Y + TAB_H/2);
}

void drawGeneralButton(bool active) {
  int x = 4 * TAB_W;
  if (active) {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(x, TAB_Y, TAB_W, TAB_H, TFT_BLACK);
    drawScanlinesButtons(x, TAB_Y, TAB_H, TAB_W);
    tft.drawRect(x, TAB_Y, TAB_W, TAB_H, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  tft.setTextDatum(MC_DATUM);
  tft.drawString("GENERAL", x + TAB_W/2, TAB_Y + TAB_H/2);
}


void updateHPAP() {
    time_t current = now();
    time_t local = myTZ.toLocal(current, &tcr);
    int h = hour(local);
    int m = minute(local);
    int minutes = h * 60 + m;
    
    // ==================== HP ====================
    // Дневной минимум HP = hpMax * 80/420 ≈ 19% от максимума
    int hpMin = hpMax * 80 / 420;  
    
    if (minutes >= 480 && minutes <= 1200) {        // 08:00 - 20:00
        currentHP = hpMax - (hpMax - hpMin) * (minutes - 480) / 720;
    } else if (minutes > 1200 || minutes < 180) {   // 20:00 - 03:00
        int since20 = (minutes > 1200) ? (minutes - 1200) : (minutes + 240);
        currentHP = hpMin + (hpMax - hpMin) * since20 / 420;
        if (currentHP > hpMax) currentHP = hpMax;
    } else {
        currentHP = hpMax;                          // 03:00 - 08:00
    }
    // ==================== AP ====================
    // Промежуточные пороги от apMax
    int hpNoon = apMax * 60 / 420;    // ~14% от макс (было 60 при 420)
    int hpNight = apMax * 30 / 420;   // ~7% от макс (было 30 при 420)
    
    if (minutes >= 480 && minutes <= 780) {       // 08:00 - 13:00
        currentAP = apMax - (apMax - hpNoon) * (minutes - 480) / 300;
    } 
    else if (minutes > 780 || minutes < 300) {    // 13:00 - 05:00
        int since13 = (minutes > 780) ? (minutes - 780) : (minutes + 660);
        currentAP = hpNoon - (hpNoon - hpNight) * since13 / 960;
        if (currentAP < hpNight) currentAP = hpNight;
    }
    
    // 05:00 - 05:30: финальный спад до 0
    if (minutes >= 300 && minutes <= 330) {
        currentAP = hpNight - hpNight * (minutes - 300) / 30;
    }
    
    // После 05:30: восстановление по 1 каждые 5 минут
    if (minutes > 330 && minutes < 480) {
        currentAP = (minutes - 330) / 5;
        if (currentAP > apMax) currentAP = apMax;
    }

    Serial.printf("HP %d/%d AP%d/%d, curr minutes:%d\n", currentHP, hpMax, currentAP, apMax, minutes);
    if (currentScreen == 0)
    {
      tft.fillRect(132, 8, 320 - 3 - 132, 10, TFT_BLACK);
      drawScanlinesButtons(132, 5, 14, 320 - 3 - 132);
      tft.setTextColor(TFT_GREEN);
      tft.setTextSize(1);
      tft.setTextDatum(MC_DATUM);
      tft.setCursor(135, 10);
      tft.printf("HP %d/%d\n", currentHP, hpMax);
      tft.setCursor(210, 10);
      tft.printf("AP %d/%d", currentAP, apMax);
        if (currentHP == hpMax)
        {
          tft.setCursor(280, 10);
          tft.print("XP MAX");
        }
    }
}



