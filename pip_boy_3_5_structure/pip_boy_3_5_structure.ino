/*
 * ESP32 3.5" 320x480 TFT ST7796 + Resisteve Touch 
 * ESP32 2.4" 240x320 TFT ILI9341 + Resisteve Touch 
 + RTC DS1307 
 + AT24C32 EEPROM 
 + GPS NEO-6M 
 + PulseMeter MAX30102 
 + LaserDistance Meter TOF10120

 * Стилизованный Pip-Boy из Fallout

ARDUINO IDE PREFERENCES:
  ESP32 DEV Module - COM Port USB

  Flash mod: DIO
  Partition scheme: NoOta 2mb APP / 2mb SPIFFS
  Events run: core1
  Flash frequency: 80Mhz
  Arduino runs: core1
  Upload speed: 460800 (choose max speed)
  PSRAM: Disabled

* ======== Boards: ======== 
  ESP32 by Espressif Systems            ver 3.3.7

* ======== Libraries: ======== 
  RTClib by Adafruit                    ver 2.1.4
  Timezone Library by Jack Christensen  ver 1.2.6
  ESP8266Audio by Early F.Philhover     ver 2.4.1
  Time by Michael Margolis              ver 1.6.1 
  ArduinoJson by Benoit Blanchon        ver 7.4.3 
  TFT_eSPI by Boodmer                   ver 2.5.43
  Adafruit BusIO by Adafruit            ver 1.17.4  
  Adafruit BMP280 Library by Adafruit   ver 3.0.0
  Adafruit BME280 Library by Adafruit   ver 2.3.0
  PNGDec by Larry Bank                  ver 1.1.6
  SparkFun MAX3010x pulse and Proximity ver 1.1.2
  TinyGPSPlus by Mikal Hart             ver 0.0.4

* ======== See config.h for base setup.  ======== 
Расскоментировать одну из строк, в зависимости от того какая плата используется
  //  #define CYD2_4          // Uncomment if you use 2.4 CYD
   // #define CYD3_5          // Uncomment if you use 3.5 CYD

  PERSON_NAME - Name in main screen
  DEBUGFLAG - if you need debug. Set flag true for first run.
    Use Serial monitor to watch Debug information.
  GMT_SET - to set GMT region for clock
  Default Password for WiFI in var StandartWiFiPass in main *.ino file  (only for first use)
  Default folder from SD card for mp3 in RadioModule.cpp  (only for first use)

* ====== components =========
  CYD: 2.4 inch ESP32-2432S024R with Resistive touch (Only type-c usb connector, RGB led in Front near display. )
  OR CYD: ESP32 3.5 inch 320x480 with Resisteve Touch (Only micro usb connector, RGB led in Front near display. )
  RTC + EEPROM :   Tyni RTC I2C module DS1307
  Connectors:      JST 1.25 4pin - for i2c bus
                   JST 1.25 2pin (2 pcs)  - for dinamic and battery
  Audio: 4 Omh speaker
  ! MicroSD card 1-16 GB for Radio via mp3 & for cash map png tiles

 Для работы часов нужен модуль с флэшкой памяти (на модуле должно быть две 8 ногих микросхемы)/ либо флэшка памяти отдельно i2c

 PULSE MAX30102 or MAX30105 (better) На главном экране HP и AP  меняется на BPM и SpO2 значения с датчика, если он распознал руку
 GPS NEO-6M (подключение 3 варианта < смотреть config.h > : UART через FPC разъем  1 = 3.3v, 3 = TX GPS, 4 = RX GPS, 6 = GND
                                                            BRIDGE через переходник SC16IS750 UART -> i2c 
                                                            i2c через ESP32C3SuperMini )
 LaserDistance Meter TOF10120 (подключение через i2c) на экране с картой будет кнопка с активацией дальномера

 динамик ( можно от мобильника 4 Ом , например с старых айфонов)
 модуль сенсора пульса (опционально) пока не реализовано
 флэшка microSD (до 32 гб, чем меньше тем лучше. оптимально - 8 гб) FAT32. С нее считывается музыка локально. Радиостанции из фалаут можно найти отдельно.

* ====== функции ======
пароль вай вай запоминает последний введенный,
координаты GPS и выбор температры( Celsius - Farengheit) запоминает последние введенные

папку с MP3 файлами так же запоминает введенную (по умолчанию SD:/mp3/)

Функция часов. Синхронизация времени через WiFi
Воспроизведение радио через WiFi или с SD карты в фоне
3 таймера с отсчетом в минутах до указанного времени (стилистически на главном экране справа отображается как список предметов и количество)
Несколько картинок состояния персонажа для главного экрана
Функция отображения погоды по заданным координатам местности
Функцим WiFi: сканирование, список сетей, подключение к выбранной (Подключать надо вручную всегда)
Индикаторы HP - AP высчитываются сами, в зависимости от врмени суток. В течение дня - уменьшаются. После вечера - восполняются.

* ====== на главном экране ======
Слева в углу 4 состояния:
в рамке - активно

wifi - подключен ли WiFi

Rad - идет ли воспроизведение музыки

W индикатор погоды:
W Err - данных о погоде нет
W [E] - данные считанные из EEPROM
W [O] - Данные актуальные из OpenMeteo
W [W] - Данные актуальные из WTTR

GPS - подключен ли GPS 

Справа (ограничен тремя строками): список предметов ( настраивается в general - time)
Служит для отображения количества минут до определенного времени (таймер) (в минутах, ограничено max 240)
Конструкция строк: (сколько осталось минут) название


HP - индикатор пульса при подключенном датчике пульса
HP - зависит от времени суток (с 8 утра до 8 вечера уменьшается с 420 до 80) Потом восстанавливается до максимума к 03:00
AP - зависит от времени суток (с 8 утра до 13:00 уменьшается до 60, потом до 5:00 уменьшается до 30.
 Потом с 5:00 до 5:30 уменьшается до 0 каждую минуту отнимаеся 1) Потом восстанавливается до максимума каждые 5 минут.


 */

#include "config.h"
#include "rtc_module.h"
#include "eeprom_module.h"
#include "wifi_module.h"
#include "weather_module.h"
#include "ui_module.h"
#include "radio.h"
#include "BMP280Module.h"
#include "pulse.h"
//#include "GPSModule.h"
#include "gps_interface.h"
#include "maps.h"
#include "tof10120.h"

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

//date time update slot 0

//Weather slot 1
struct BackUpDataGPS {    //slot 1
  char GPS_LAT[15];        // 15 байт
  char GPS_LON[15];              // 15 байт
  uint8_t icCels; 
} __attribute__((packed));   // <-- БЕЗ ПАДДИНГА!

//WiFi BackUpDataWiFi //slot 2
struct BackUpDataWiFi {   //slot 2
  char WiFi_Pass[32];           // 32 байт
} __attribute__((packed));   // <-- БЕЗ ПАДДИНГА!

//mp3 folder slot 3
struct BackUpDataFolder {   //slot 3
  char SDFolder[32];           // 32 байт
} __attribute__((packed));   // <-- БЕЗ ПАДДИНГА!

//timers slot 4
struct BackUpTimers {       //slot 4
  char T1Name[8]; // 8 байт
  char T2Name[8]; 
  char T3Name[8];  
  uint8_t T1hour;
  uint8_t T2hour;
  uint8_t T3hour;
  uint8_t T1min;
  uint8_t T2min;
  uint8_t T3min;   
} __attribute__((packed));   // <-- БЕЗ ПАДДИНГА!

//sync date time slot 5
struct LastSyncDateTime {     // SLOT 5
  unsigned long lastSyncDT;    // millis() когда получено         
};

time_t eepromUpdateDataTime = 0;

// Калибровка тачскрина (rotation 1) 
// uint16_t calData[5] = { 178, 3762, 323, 3401, 1 };



//CYD 2.4 (rotation 3) 
#ifdef CYD2_4
  uint16_t calData[5] = { 318, 3540, 321, 3449, 6 };  
#endif

 //CYD 3.5 (rotation 3) 
#ifdef CYD3_5
  uint16_t calData[5] = { 160, 3766, 282, 3404, 7 }; 
#endif

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
int timeY = SCREEN_CONTENT_H / 3; // Позиция часов на экране
bool needUpdateTimeScreen = false;
int prevDay = 0;

// переменные таймеров
bool timeSettingsActive = false;
uint8_t T1h = 13, T1m = 00, T2h = 17, T2m = 30, T3h = 23, T3m = 00;
String T1S = "Food", T2S = "Stimpack",  T3S = "";
String EditFieldFlag = "";
uint8_t parsedHours = 99;
uint8_t parsedMinutes = 99;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

// BMP
bool bmpFound = false;

// PULSE SENSOR  Available flag
bool pulseSensFound = false;
bool psActive = false;

/* // GPS
#ifdef CYD2_4
  GPSModule gps(Serial,  GPS_ONRX_PIN, GPS_ONTX_PIN, 9600);
#endif

#ifdef CYD3_5
  GPSModule gps(Serial2, GPS_ONRX_PIN, GPS_ONTX_PIN, 9600);
#endif
*/

// GPS Available flag
bool GPS_Connected = false;

MapsModule* pipMaps = nullptr;
float gpsLat = atof(DEFAULT_LAT);
float gpsLon = atof(DEFAULT_LON);
bool GPSZoomOut = false;
 //SPIClass SDSPI(VSPI);

setup_t user;

int currentScreen = 0;
int lastScreen = -1;  //mp3/
int vaultFrame = 1;

// General screen
int ButtonScreen2 = 0;
bool weatherSettingsActive = false;
bool needUpdateScreenWeather = false;
bool editlat = false;
bool editlon = false;

int currentHP = 420;
int currentAP = 420;
int apMax = 210;   // Максимум AP (можно менять, например, при прокачке персонажа)
int hpMax = 320;   // Максимум HP

// laser TOF  Available flag
bool laserModule = false;
bool laserActive = false;

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

// ============================================================
// STARTUP CONSOLE — автопрокрутка + посимвольная печать
// ============================================================


TFT_eSprite startupSprite(&tft);

static int16_t  su_cx = 0;
static int16_t  su_cy = 0;
static uint16_t su_delayMs = 0;
static uint8_t  su_font = 1;
static uint8_t  su_size = 1;
static int16_t  su_lineH = 8;

static char     su_buf[6];   // буфер на 5 символов + '\0'
static uint8_t  su_bufLen = 0;


// ======================= ПРОТОТИПЫ ФУНКЦИЙ =======================

//  ============= handles on touchscreen =====================
void handleTouch(uint16_t x, uint16_t y);
void HandleButtonsScreen4(uint16_t x, uint16_t y);
void handleWiFiTouch(uint16_t x, uint16_t y);
void handleRadioSettingsTouch(uint16_t x, uint16_t y);
void HandleTimeSettings(uint16_t x, uint16_t y);
void handleWeatherSettingsTouch(uint16_t x, uint16_t y);
void handleRadioSetButtons(uint16_t x, uint16_t y);
bool handleKeyboardTouch(uint16_t x, uint16_t y);
void handleButtonScreen5(uint16_t x, uint16_t y);
// =========================== main functions =======================
void initI2C();
bool LoadBackUpFromEPPR();
bool SaveBackUpToEPPR();
void ShowTFTUserSetup();
void initStartUp();
void drawWindArrow();
void drawScanlines();
void drawScanlinesButtons(int16_t xS, int16_t yS, int16_t hS, int16_t wS);
void drawPipBoyScreen(); // stats
void drawPipBoyScreen1(); //clock
void drawPipBoyScreen2(); // radio
void drawPipBoyScreen3(); //weather
void drawPipBoyScreen4(); // GEneral setup
void drawPipBoyScreen5(); // GPS
void drawButtonsScreen4();
void drawRadioSetButtons();
void updateHPAP();
void UpdateLeftPanel();
void UpdateRightPanel();
bool parseTime(const char* input);
void sanitizeTimeInput(char* input);
void sanitizeGPSInput( char* input);
bool isValidString(const String& str);
String pad2(uint8_t val);

///General Setup Screen

void drawRadioSettings();
void drawTimeSettings();
void drawWeatherSettings();


void drawTabButtons();
void drawStatusButton(bool active);
void drawSpecialButton(bool active);
void drawSkillsButton(bool active);
void drawWeatherButton(bool active);
void drawGeneralButton(bool active);
void drawVaultBoy(int16_t cx, int16_t cy, int8_t frame);

void drawWiFiScreen();
void scanWiFiNetworks();
void connectToWiFi(const char* ssid, const char* password);

// GPS
void UpdateMapInfoPanel();
// Клавиатура
void initKeyboard();
void drawKeyboard();
void drawKey(Key* k, bool pressed);
void redrawInputField();
void showKeyboard(const char* placeholder);
void hideKeyboard();
const char* getKeyboardInput();
void clearKeyboardInput();


// bmp module barometr and temp. -- барометр и температура
//bool bmpModuleInit();
bool bmpInit();
bool bmpMeasure();
float bmpGetPressureHpa();
float bmpGetPressureKpa();
float bmpGetPressureMmHg();
float bmpGetTemperatureC();
float bmpGetTemperatureF();
void bmpCalibrateAltitude();      // Запомнить текущее как "ноль"
bool bmpIsCalibrated();
float bmpGetRelativeAltitude();   // От калиброванной точки
float bmpGetSeaLevelAltitude();   // От уровня моря
void bmpSetSeaLevelPressure(float hpa); // Установить P моря (метео)

void updateBMPScreen();
void drawUpdateInfo();
void drawWeatherPanel(int x, int y, int w, int h, bool isSidePanel);
void drawBMPPanel(int x, int y, int w, int h);

void updateWeatherScreen();

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
void syncTimeFromGPS();
bool NeedSyncDataTime();
void SyncSaveToEEPROM();
bool SyncLoadFromEEPROM();

// погода
bool isDayTime();
time_t weatherLastUpdate();
void drawWeatherIcon(int x, int y, String condition, uint16_t color);
void drawCurrentWeatherIcon(int x, int y, uint16_t color);
void drawWeatherIconCentered(int x, int y, String condition, uint16_t color);
void drawCurrentWeatherIconCentered(int x, int y, uint16_t color);
void updateWeatherScreen();

// radio
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

// WiFi
void wifiInit();
void scanWiFiNetworks();
void drawWiFiScreen();
void connectToWiFi(const char* ssid, const char* password);
void wifiDisconnect();
bool wifiIsConnected();

// Buzzer
void geigerTick(int level);
void startBuzzer();
void sweepTone(int startFreq, int endFreq, int durationMs, int stepMs);
void clackBuzzer();
void clickBuzzer();
bool pulseSensorUpdate();
void clickBuzzerKey();

// ======================= SETUP =======================
void setup() {
  if (DEBUGFLAG)
  {
    Serial.begin(115200);
    delay(1000);
  }
  
  if (DEBUGFLAG) Serial.println("Pip-Boy starting...");
  
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
  if (SD.begin(SD_CS, SDSPI, 2000000)) {                // CS = IO5 (по твоей схеме)
    sdCardInitialized = true;
    if (DEBUGFLAG) Serial.println("SD Card Initialized 2MHz");
  } else if (SD.begin(SD_CS, SDSPI, 1000000))
  {                
    sdCardInitialized = true;
    if (DEBUGFLAG) Serial.println("SD Card Initialized 1MHz");
  }
  else {
    sdCardInitialized = false;
    if (DEBUGFLAG) Serial.println("SD Card mount failed ERROR");
  }

  if (sdCardInitialized)
  {
      uint8_t cardType = SD.cardType();

      if (cardType == CARD_NONE) {
        if (DEBUGFLAG) Serial.println("No SD card attached");
        return;
      }
        if (DEBUGFLAG) Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    if (DEBUGFLAG) Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    if (DEBUGFLAG) Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    if (DEBUGFLAG) Serial.println("SDHC");
  } else {
    if (DEBUGFLAG) Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  if (DEBUGFLAG) Serial.printf("SD Card Size: %lluMB\n", cardSize);

  }
  
   //gps.begin();
    //if (DEBUGFLAG) Serial.println("[GPS] UART инициализирован");
    
    
  // TFT

  //if (BUZZER_PIN == LED_B)
  //pinMode(35, INPUT_PULLUP);
  //pinMode(35, INPUT);


  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
  
  /*
  if (!tft.begin(TFT_BLACK))
  {
    if (DEBUGFLAG) Serial.print("FAIL to init TFT!\n\n")
    return;
  }
  */
  tft.init();
  tft.setSwapBytes(false);
  tft.setRotation(3);
  
  tft.fillScreen(TFT_BLACK);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  pinMode(BUZZER_PIN, OUTPUT);
  randomSeed(analogRead(0)); // или randomSeed(millis())

  tft.setTouch(calData);

  //проверка настроек дисплея
  if (DEBUGFLAG) ShowTFTUserSetup();

  
  // Запускаем сразу для теста
  //pipboyBootSound();

      // Инициализация модулей
  initStartUp();
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
  tft.drawBitmap((TFT_WIDTH_SCREEN - 317)/2, (TFT_HEIGHT_SCREEN - 47 - 64 - 30)/ 2, labelpip317x47, 317, 47, TFT_GREEN);
  
  for (int i = 0; i < 7; i++) {
    tft.fillRect((TFT_WIDTH_SCREEN - 100)/2 - 32,  (TFT_HEIGHT_SCREEN - 64)/ 2 + 10, 128, 64, TFT_BLACK);
    drawScanlinesButtons((TFT_WIDTH_SCREEN - 100)/2 - 32, (TFT_HEIGHT_SCREEN - 64)/ 2 + 10, 64, 128);
    tft.drawBitmap((TFT_WIDTH_SCREEN - 100)/2 - 32, (TFT_HEIGHT_SCREEN - 64)/ 2 + 10, vaultBoyFrames[i], 128, 64, TFT_GREEN);
    delay(250);
  }
  
  delay(2000);
  
  // Первый экран
  drawPipBoyScreen();
  drawTabButtons();
  
  if (DEBUGFLAG) Serial.println("Pip-Boy ready!");
}

// ======================= LOOP =======================
void loop() {
  uint16_t xTouch, yTouch;

  geigerTick(vaultFrame);
  if (GPS_Connected == true)
  {
    gpsUpdate();
  }
  
  static unsigned long lastBpmUpdate = 0;
  
  static unsigned long lastHPAPUpdate = 0;

  if (pulseSensFound && currentScreen == 0)
  {
       pulseUpdate(); 
       pulseDrawGraph(TFT_WIDTH_SCREEN - 61, 2, TFT_GREEN);  
       //pulseDrawGraph(260, 2, TFT_GREEN);  
  }

  if (millis() - lastHPAPUpdate > 1000) {
            lastHPAPUpdate = millis();
            
    if (GPS_Connected == true)
    {
      if (currentScreen == 4)
        UpdateMapInfoPanel();
      if (DEBUGFLAG && GPS_Connected) 
        {
        String srcg = "[GPS] Satt: 000 On: 0";
        if (gpsHasFix()) {
                syncTimeFromGPS();
                srcg = "[GPS] Active T: " + String(gpsGetHour()) + ":" + String(gpsGetMinute()) + ":" + String(gpsGetSecond());
        } 
        // Вариант 3: Полная тишина
        else {
            srcg = "[GPS] No data from GPS";
        }

        
          tft.setTextSize(1);
          tft.setTextDatum(TL_DATUM);
          tft.setTextColor(TFT_GREEN);
          // Закрасить старое значение
          //String src = "[GPS] Active D: 00.00.00";
          
          String src = "[GPS] Satt: " + String(gpsGetSats()) + " Acc: " + String(gpsGetHdop());
          int tws = tft.textWidth(src); 
          tft.fillRect(5, SCREEN_BOTTOM_Y - 30 - TAB_H, tws, 22, TFT_NAVY);
         // tft.setCursor(7, SCREEN_BOTTOM_Y - 29 - TAB_H);
          tft.drawString(srcg, 5, SCREEN_BOTTOM_Y - 30 - TAB_H );
          //tft.setCursor(7, SCREEN_BOTTOM_Y - 15 - TAB_H);
          tft.drawString(src, 5, SCREEN_BOTTOM_Y - 16 - TAB_H );
        }
    }

    if (pulseSensFound && currentScreen == 0)
  {
    pulseSensorUpdate();
  }

  }


  if (laserModule && currentScreen == 4 && laserActive)
    {
        static unsigned long lastTofCheck = 0;
        if (millis() - lastTofCheck > TOF10120_READ_INTERVAL) {
        lastTofCheck = millis();
        int laserDist = tofSensor.readDistanceFiltered();
        if (laserDist != TOF10120_ERROR_VALUE)
        {
          tft.setTextDatum(TR_DATUM);
          tft.setTextSize(1);
          tft.setTextColor(TFT_GREEN);
          tft.fillRect(TFT_WIDTH_SCREEN - 80 , 5, 80, 10, TFT_BLACK);
          String laserStr = "Dist: " + String(laserDist) + "mm.";
          tft.drawString(laserStr, TFT_WIDTH_SCREEN, 7);
        }
        }
    }


// Обновление карты каждые 10 сек
  static unsigned long lastMapUpdate = 0;
  static float gpsLat_last = atof(DEFAULT_LAT);
  static float gpsLon_last = atof(DEFAULT_LON);

  if (millis() - lastMapUpdate > 10000) {
    lastMapUpdate = millis();

    if (currentScreen == 0)
      {
          UpdateLeftPanel();
          updateHPAP();
          UpdateRightPanel();
      }
    

    if (currentScreen == 4)
    {
        int16_t  pbW = ( TFT_WIDTH_SCREEN / 2);
        uint16_t pbH = 8;
        uint16_t pbX = ( TFT_WIDTH_SCREEN / 2) - pbW/2;
        uint16_t pbY = MAP_START_Y - 9;
      if (GPS_Connected == true && gpsHasFix())
      {
        gpsLat= gpsGetLat();
        gpsLon= gpsGetLon();
        float dLat = abs(gpsLat - gpsLat_last);
        float dLon = abs(gpsLon - gpsLon_last);
      
      if (GPSZoomOut)
        pipMaps->setZoom(MAP_ZOOM_OUT);
      else
      {
          if ((dLat > 0.01f || dLon > 0.01f))
          {
            if (DEBUGFLAG) Serial.println("[GUI] Auto zoom out");
            pipMaps->setZoom(MAP_ZOOM_OUT);
          }
          else
          {
            if (DEBUGFLAG) Serial.println("[GUI] Auto zoom in");
            pipMaps->setZoom(MAP_ZOOM_IN);
          }
      }
      if ((dLat > 0.001f || dLon > 0.001f && pipMaps->getZoom() == MAP_ZOOM_IN) || (dLat > 0.01f || dLon > 0.01f && pipMaps->getZoom() == MAP_ZOOM_OUT))
      {
      uint8_t have = pipMaps->cachedCount(gpsLat, gpsLon, pipMaps->getZoom());
    // --- Экран загрузки ---
    
    if (have < 9 && WiFi.status() == WL_CONNECTED) {
        
        //статус подгрузки карты
        tft.fillRect(pbX, pbY, pbW, pbH, TFT_BLACK);
        pipMaps->ensureTiles(gpsLat,gpsLon,pipMaps->getZoom() , [&](uint8_t p) {
            tft.fillRect(pbX + 2, pbY + 2, (p * (pbW - 4)) / 100, pbH - 4, TFT_GREEN);
        });

        tft.fillRect(pbX, pbY, pbW, pbH, TFT_BLACK);
        //delay(20);
        lastMapUpdate = millis();
    }
        //pipMaps->drawMap(gpsLat, gpsLon);
        gpsLat_last = gpsLat;
        gpsLon_last = gpsLon;

        tft.fillRect(pbX, pbY, pbW, pbH, TFT_BLACK);
        if (DEBUGFLAG) Serial.println("[GUI] Redraw Map");
         pipMaps->redrawMap(gpsLat, gpsLon);
      }      

      // загружаем доп тайлы вокруг точки
      static unsigned long lastExtLoad = 0;
      if (!pipMaps->isLoading() && WiFi.status() == WL_CONNECTED) {
        if (millis() - lastExtLoad > 10000) {
            lastExtLoad = millis();
            pipMaps->ensureExtendedTiles(gpsLat,gpsLon,pipMaps->getZoom() , [&](uint8_t p) {
              tft.fillRect(pbX + 2, pbY + 2, (p * (pbW - 4)) / 100, pbH - 4, TFT_GREEN); });
        }
      }
        tft.setTextDatum(TC_DATUM);
        tft.setTextSize(2);
        if (pipMaps->getZoom() == MAP_ZOOM_OUT)
        {
          GPSZoomOut = true;
          tft.setTextColor(TFT_GREEN);
          tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_BLACK);
          tft.drawRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.drawString("+", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13);
        } else
        {
          GPSZoomOut = false;
          tft.setTextColor(TFT_BLACK);
          tft.drawRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.drawString("+", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13);  
        }
      }
    }
  }
  // Обновление времени погоды каждую минуту
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 60000) {
    lastTimeUpdate = millis();
    if (!weatherHasData() ) 
    {
      weatherUpdate();
    }

    if (!weatherHasData() || (WiFi.status() == WL_CONNECTED))
      {
        int mins = weatherGetAgeMinutes();
        if (mins > 60)
        {
          //needUpdateScreenWeather = true;
          weatherForceUpdate();
          weatherUpdate();
        }
      }
    
    if (currentScreen == 3 && weatherHasData()) {
      if (needUpdateScreenWeather) drawPipBoyScreen3();
      drawUpdateInfo();
      if (weatherGetAgeMinutes() > 15)
      {
        weatherForceUpdate();
        weatherUpdate();
        needUpdateScreenWeather = true;
      }
    }
    
    

    if (currentScreen == 2)
    {
       
        if (DEBUGFLAG) Serial.println("Radio screen working..");
        UpdateMetaData();
    }


  if (GPS_Connected && DEBUGFLAG)
  {

    if (gpsHasFix()) {
        Serial.printf("[GPS] Sats: %d | Lat: %.6f | Lng: %.6f | Acc: %.1f | Speed: %.1f km/h | Time: --\n",
            gpsGetSats(),
            gpsGetLat(),
            gpsGetLon(),
            gpsGetHdop(),
            gpsGetSpeedKmph());
    }
    // Вариант 3: Полная тишина
    else {
        Serial.println("[GPS] ERROR - NO DATA ");
    }
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
    if (needUpdateTimeScreen)
    {
      drawPipBoyScreen1();
      needUpdateTimeScreen = false;
    }
    time_t current = now();
    if (current != prevDisplay) {
      digitalWrite(LED_R, LOW);
      delay(30);
      digitalWrite(LED_R, HIGH);
      prevDisplay = current;
      ParseDigits(prevDisplay);
      DrawDigitsOneByOne();
      DrawDate(prevDisplay);
      //DrawColons();
      DrawAmPm();
      TimeSourceUpdate();
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
        tft.drawFastVLine(KEYBOARD_X + KEY_W * 2 + KEY_GAP*2 + 5 + 6 + textW, KEYBOARD_Y + 32, 14, TFT_GREEN);
      else
        tft.drawFastVLine(KEYBOARD_X + KEY_W * 2 + KEY_GAP*2 + 5 + 6 + textW, KEYBOARD_Y + 32, 14, TFT_BLACK);
    }
  }
  
  // Обработка Enter с клавиатуры
  if (KeyboardEnter) {
    if (wifiWaitingForPassword) {
      wifiWaitingForPassword = false;
      const char* inputWPs = getKeyboardInput();
      //memset(inputWPs, 0, sizeof(inputWPs));
      memcpy(StandartWiFiPass, inputWPs, 32);
      //StandartWiFiPass[sizeof(inputWPs - 1)] = '\0';
      SaveBackUpToEPPR();
      connectToWiFi(wifiTargetSSID, inputWPs);
      delay(100);
    }
    if(weatherSettingsActive)
    {
      
      if (editlat) 
      {
        editlat = false;
        const char* inputLat = getKeyboardInput();
        char buf[12];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, inputLat, 12);

        if (DEBUGFLAG) Serial.printf("Enter weatherLat: %s", buf);
        sanitizeGPSInput(buf);
        if (DEBUGFLAG) Serial.printf(" After Format: %s", buf);
        if (buf[0] != 0 && (buf[0] !=  '.'))
        {
          weatherLat = String(buf);
          SaveBackUpToEPPR();
        }
      }
      if (editlon) 
      {
        editlon = false;
        const char* inputLon = getKeyboardInput();
        char buf[12];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, inputLon, 12);

        if (DEBUGFLAG) Serial.printf("Enter weatherLon %s", buf);
        sanitizeGPSInput(buf);
        if (DEBUGFLAG) Serial.printf(" After Format: %s", buf);
        if (buf[0] != 0 &&  (buf[0] !=  '.'))
        {
          weatherLon = String(buf);
          SaveBackUpToEPPR();
        }
      }
      drawWeatherSettings();
    }
    
    if (timeSettingsActive)
    {
      const char* inputTimeField = getKeyboardInput();
      String EditFieldTimer = String(inputTimeField).substring(0, 8);
      //memcpy(EditFieldTimer, inputTimeField, 32);
      if (EditFieldTimer)
        if (EditFieldFlag == "T1S") T1S = EditFieldTimer; 
        if (EditFieldFlag == "T2S") T2S = EditFieldTimer;
        if (EditFieldFlag == "T3S") T3S = EditFieldTimer;

        if (EditFieldFlag == "T1h"){
          if (parseTime(inputTimeField)) {
            T1h = parsedHours;
            if (parsedMinutes != 99) T1m = parsedMinutes;
          }
         }
        if (EditFieldFlag == "T2h"){
          if (parseTime(inputTimeField)) {
            T2h = parsedHours;
            if (parsedMinutes != 99) T2m = parsedMinutes;
          }
         }
        if (EditFieldFlag == "T3h"){
          if (parseTime(inputTimeField)) {
            T3h = parsedHours;
            if (parsedMinutes != 99) T3m = parsedMinutes;
          }
         }
        if (EditFieldFlag == "T1m") 
          if (parseTime(inputTimeField)) T1m = parsedMinutes;
        if (EditFieldFlag == "T2m") 
          if (parseTime(inputTimeField)) T2m = parsedMinutes;
        if (EditFieldFlag == "T3m") 
          if (parseTime(inputTimeField)) T3m = parsedMinutes;

       //if  parseTime();
       EditFieldFlag= "";
      drawTimeSettings();
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
        tft.fillRect(0, 42, TFT_WIDTH_SCREEN, 102, TFT_BLACK);  // очистка области      
        drawScanlinesButtons(0, 42, 102, TFT_WIDTH_SCREEN);
        tft.setTextSize(2);

        //String RadioInfo = "Radio: " + String(radioGetStationName());
        String RadioInfo = String(radioGetStationName());
        
        tft.drawString(RadioInfo, TFT_WIDTH_SCREEN / 2, 42);
        if (radioIsPlaying())
          {
            tft.drawString(radioGetCurrentArtist(), TFT_WIDTH_SCREEN / 2, 70);
            tft.drawString(radioGetCurrentTitle(), TFT_WIDTH_SCREEN / 2, 100);
          }
        //tft.drawString(radioGetCurrentSong(), TFT_WIDTH_SCREEN / 2, 100);
      }

      if (radioPlaySource == 0 )
      {
        
          tft.fillRect(0, 42, TFT_WIDTH_SCREEN, 102, TFT_BLACK);  // очистка области      
          drawScanlinesButtons(0, 42, 104, TFT_WIDTH_SCREEN);
          //if (radioIsPlaying())
          tft.setTextSize(2);
          tft.drawString(radioGetSDTrackName(), TFT_WIDTH_SCREEN / 2, 42);
          if (radioIsPlaying())
          {
          tft.drawString(radioGetCurrentArtist(), TFT_WIDTH_SCREEN / 2, 70);    
          tft.drawString(radioGetCurrentTitle(), TFT_WIDTH_SCREEN / 2, 100);
          }
      }

        tft.setTextSize(1);
        String VolumeInfo = "Volume: 100";// + String(radioGetVolume());
        int widthtext = tft.textWidth(VolumeInfo);
        int cvy = SCREEN_BOTTOM_Y - 20 - TAB_H;
                  tft.fillRect((TFT_WIDTH_SCREEN / 2) - (widthtext/2), cvy, widthtext + 5, 10, TFT_BLACK);
                  drawScanlinesButtons((TFT_WIDTH_SCREEN / 2) - (widthtext/2), cvy, 12, widthtext + 5);
                  tft.drawString("Volume: " + String(radioGetVolume()), TFT_WIDTH_SCREEN / 2, cvy);
         // }
    }


}

////// =====================
void ShowTFTUserSetup()
{
  tft.getSetup(user); //
  Serial.print("========== DISPLAY INFO: ===========\n\n");
  if (user.tft_driver != 0xE9D) // For ePaper displays the size is defined in the sketch
{
  Serial.print("Display driver = "); Serial.println(user.tft_driver, HEX); // Hexadecimal code
  if (user.tft_width == TFT_HEIGHT_SCREEN) {   Serial.print("TFT_WIDTH   OK = "); Serial.print(user.tft_width); } else {                Serial.printf("\nTFT_WIDTH   = ERROR , NEED %d | But define is = ", TFT_HEIGHT_SCREEN); Serial.print(user.tft_width);}
  if (user.tft_height == TFT_WIDTH_SCREEN) {  Serial.print("\nTFT_HEIGHT  OK = "); Serial.print(user.tft_height); } else {               Serial.printf("\nTFT_HEIGHT  = ERROR , NEED %d | But define is = ", TFT_WIDTH_SCREEN); Serial.print(user.tft_height);}

 
}
else if (user.tft_driver == 0xE9D) Serial.println("Display driver = ePaper\n");

if (user.tft_driver == 30614) 
{ Serial.print("\nST7796_DRIVER  OK"); } else { Serial.print("\nST7796_DRIVER      ERROR! , UNCOMENT #define ST7796_DRIVER // "); Serial.println(user.tft_driver); }
 Serial.println();
if (user.pin_tft_mosi == 13) { Serial.print("\nMOSI        OK = "); Serial.print(getPinName(user.pin_tft_mosi)); } else { Serial.print("\nMOSI        = ERROR , NEED 13  | But define is = "); Serial.print(getPinName(user.pin_tft_mosi));}
if (user.pin_tft_miso == 12) { Serial.print("\nMISO        OK = "); Serial.print(getPinName(user.pin_tft_miso)); } else { Serial.print("\nMISO        = ERROR , NEED 12  | But define is = "); Serial.print(getPinName(user.pin_tft_miso));}
if (user.pin_tft_clk  == 14) { Serial.print("\nSCLK        OK = "); Serial.print(getPinName(user.pin_tft_clk)); } else {  Serial.print("\nSCLK        = ERROR , NEED 14  | But define is = "); Serial.print(getPinName(user.pin_tft_clk));}
if (user.pin_tft_dc   == 2 ) { Serial.print("\nDC          OK = "); Serial.print(getPinName(user.pin_tft_dc)); } else {   Serial.print("\nDC          = ERROR , NEED 2   | But define is = "); Serial.print(getPinName(user.pin_tft_dc));}
if (user.pin_tft_cs   == 15) { Serial.print("\nCS          OK = "); Serial.print(getPinName(user.pin_tft_cs)); } else {   Serial.print("\nCS          = ERROR , NEED 15  | But define is = "); Serial.print(getPinName(user.pin_tft_cs));}
if (user.pin_tch_cs   == 33) { Serial.print("\nTOUCH_CS    OK = "); Serial.print(getPinName(user.pin_tch_cs)); } else {   Serial.print("\nTOUCH_CS    = ERROR , NEED 33  | But define is = "); Serial.print(getPinName(user.pin_tch_cs));}

if (user.tft_width == TFT_HEIGHT_SCREEN) {   Serial.print("\nTFT_WIDTH   OK = "); Serial.print(user.tft_width); } else {                Serial.printf("\nTFT_WIDTH   = ERROR , NEED %d | But define is = ", TFT_HEIGHT_SCREEN); Serial.print(user.tft_width);}
if (user.tft_height == TFT_WIDTH_SCREEN) {  Serial.print("\nTFT_HEIGHT  OK = "); Serial.print(user.tft_height); } else {               Serial.printf("\nTFT_HEIGHT  = ERROR , NEED %d | But define is = ", TFT_WIDTH_SCREEN); Serial.print(user.tft_height);}


if (user.pin_tft_led == 27) {  Serial.print("\nTFT_BL      OK = "); Serial.print(getPinName(user.pin_tft_led)); } else {   Serial.print("\nTFT_BL      = ERROR , NEED 27  | But define is = "); Serial.print(getPinName(user.pin_tft_led));}

if (user.tft_spi_freq == 800){  Serial.print("\nSPI_FREQUENCY         OK = "); Serial.printf("%d00000",user.tft_spi_freq); } else {   Serial.print("\nSPI_FREQUENCY        = ERROR , NEED 80 000 000  | But define is = "); Serial.printf("%d00000",user.tft_spi_freq);}
if (user.tft_rd_freq == 800) {  Serial.print("\nSPI_READ_FREQUENCY    OK = "); Serial.printf("%d00000",user.tft_rd_freq); } else {    Serial.print("\nSPI_READ_FREQUENCY   = ERROR , NEED 80 000 000  | But define is = "); Serial.printf("%d00000",user.tft_rd_freq);}
if (user.tch_spi_freq == 25) {  Serial.print("\nSPI_TOUCH_FREQUENCY   OK = "); Serial.printf("%d00000",user.tch_spi_freq); } else {   Serial.print("\nSPI_TOUCH_FREQUENCY  = ERROR , NEED 2 500 000   | But define is = "); Serial.printf("%d00000",user.tch_spi_freq);}

Serial.print("\n=============== END of TFT Debug TFT UserSetup.h ===================\n\n");
}


int8_t getPinName(int8_t pin)
{
  // For ESP32 and RP2040 pin labels on boards use the GPIO number
  if (user.esp == 0x32 || user.esp == 0x2040) return pin;

  if (user.esp == 0x8266) {
    // For ESP8266 the pin labels are not the same as the GPIO number
    // These are for the NodeMCU pin definitions:
    //        GPIO       Dxx
    if (pin == 16) return 0;
    if (pin ==  5) return 1;
    if (pin ==  4) return 2;
    if (pin ==  0) return 3;
    if (pin ==  2) return 4;
    if (pin == 14) return 5;
    if (pin == 12) return 6;
    if (pin == 13) return 7;
    if (pin == 15) return 8;
    if (pin ==  3) return 9;
    if (pin ==  1) return 10;
    if (pin ==  9) return 11;
    if (pin == 10) return 12;
  }

  if (user.esp == 0x32F) return pin;

  return pin; // Invalid pin
}

// ============================================================
// STARTUP CONSOLE — 1‑бит спрайт, вывод пачками по 5 символов
// ============================================================



inline void suUpdateMetrics() {
    startupSprite.setTextFont(su_font);
    startupSprite.setTextSize(su_size);
    su_lineH = startupSprite.fontHeight();
    if (su_lineH <= 0) su_lineH = 8 * su_size;
}

// Ширина текущего содержимого буфера (пиксели)
inline int16_t suBufWidth() {
    if (su_bufLen == 0) return 0;
    if (su_font <= 6) return su_bufLen * 6 * su_size;  // GLCD шрифты фиксированы
    char tmp = su_buf[su_bufLen];
    su_buf[su_bufLen] = '\0';
    int16_t w = startupSprite.textWidth(su_buf);
    su_buf[su_bufLen] = tmp;
    return w;
}

// Создать монохромный спрайт на весь экран
void startupInit(uint8_t font, uint8_t textSize = 1) {
    startupSprite.setColorDepth(1);
    startupSprite.createSprite(TFT_WIDTH_SCREEN, TFT_HEIGHT_SCREEN);
    startupSprite.setBitmapColor(TFT_GREEN, TFT_BLACK);
    startupSprite.fillSprite(TFT_BLACK);

    startupSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    startupSprite.setTextWrap(false);

    su_font = font;
    su_size = textSize;
    suUpdateMetrics();

    su_cx = 0;
    su_cy = 0;
    su_bufLen = 0;
    startupSprite.pushSprite(0, 0);
}

void startupSetDelay(uint16_t ms) {
    su_delayMs = ms;
}

// Прокрутка спрайта вверх
void startupScroll(int16_t dy) {
    if (dy <= 0) return;
    startupSprite.scroll(0, -dy);
    startupSprite.fillRect(0, TFT_HEIGHT_SCREEN - dy, TFT_WIDTH_SCREEN, dy, TFT_BLACK);
    su_cy -= dy;
    if (su_cy < 0) su_cy = 0;
}

// Перевод строки с автопрокруткой
void startupNewLine() {
    su_cx = 0;
    su_cy += su_lineH;
    if (su_cy + su_lineH > TFT_HEIGHT_SCREEN) {
        int16_t need = su_cy + su_lineH - TFT_HEIGHT_SCREEN;
        if (need % su_lineH) need += su_lineH - (need % su_lineH);
        startupScroll(need);
    }
}

// Сбросить буфер в спрайт и вытолкнуть на экран
void startupFlush() {
    if (su_bufLen == 0) return;

    char tmp = su_buf[su_bufLen];
    su_buf[su_bufLen] = '\0';

    startupSprite.setCursor(su_cx, su_cy);
    startupSprite.print(su_buf);
    su_cx = startupSprite.getCursorX();

    su_buf[su_bufLen] = tmp;
    su_bufLen = 0;

    startupSprite.pushSprite(0, 0);
    
    if (su_delayMs) delay(su_delayMs);
}

// Печать одного символа с буферизацией
void startupPutChar(char c) {
    if (c == '\n') {
        startupFlush();          // сбросить то, что накопилось
        startupNewLine();
        startupSprite.pushSprite(0, 0);
        if (su_delayMs) delay(su_delayMs);
        return;
    }

    // Проверка: влезет ли символ в текущую строку?
    int16_t cw = (su_font <= 6) ? (6 * su_size) : startupSprite.textWidth(String(c));
    if (su_cx + suBufWidth() + cw > TFT_WIDTH_SCREEN) {
        startupFlush();
        startupNewLine();
    }

    su_buf[su_bufLen++] = c;

    if (su_bufLen >= 5) {
        startupFlush();
    }
}

// Печать строки посимвольно (через буфер)
void startupPrint(const char* s) {
    if (!s) return;
    while (*s) startupPutChar(*s++);
}

// Строка + перевод каретки
void startupPrintln(const char* s) {
    startupPrint(s);
    startupPutChar('\n');
}

// printf-стиль
void startupPrintf(const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    startupPrint(buf);
}

// Освободить память (сбрасывает остаток буфера перед удалением)
void startupRelease() {
    startupFlush();
    startupSprite.deleteSprite();
}


void initI2C() {
  Wire.begin(RTC_SDA, RTC_SCL);
  byte error, address;
  int nDevices = 0;

  // startupSetColor(TFT_GREEN);
  startupPrintln("Scanning for I2C devices ...");
  if (DEBUGFLAG) Serial.print("============ Scanning for I2C devices ... ============\n");

  //startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(20);

  for (address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      // startupSetColor(TFT_GREEN);

      startupNewLine();
      clackBuzzer();
      if (DEBUGFLAG) Serial.printf("I2C device found at address   0x%02X\n", address);
      startupPrintf("I2C device found at address   0x%02X\n", address);
      //startupNewLine();
      startupSprite.pushSprite(0, 0);
      //delay(20);
      nDevices++;
    } else if (error != 2) {
      // startupSetColor(TFT_RED);
      clackBuzzer();

      startupNewLine();
      if (DEBUGFLAG) Serial.printf("Error %d at address           0x%02X\n", error, address);
      startupPrintf("Error %d at address           0x%02X\n", error, address);
      //startupNewLine();
      startupSprite.pushSprite(0, 0);
      //delay(2000);
    }
  }

  if (nDevices == 0) {
    // startupSetColor(TFT_RED);
    startupNewLine();
    if (DEBUGFLAG) Serial.print("No I2C devices found!\n");
    startupPrintln("No I2C devices found.");
    //startupNewLine();
    startupSprite.pushSprite(0, 0);
    //delay(2000);
  }

  if (DEBUGFLAG) Serial.println("============== END OF I2C SCANNER ===============\n");
  // startupSetColor(TFT_GREEN);
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
}





void initStartUp() {
  // --- Логотип и анимация (рисуем сразу на экран) ---
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  tft.drawBitmap((TFT_WIDTH_SCREEN - 180) / 2, (TFT_HEIGHT_SCREEN - 80) / 2,
                 logovt180x80, 180, 80, tft.color565(0, 80, 0));
  startBuzzer();
  tft.drawBitmap((TFT_WIDTH_SCREEN - 180) / 2, (TFT_HEIGHT_SCREEN - 80) / 2,
                 logovt180x80, 180, 80, tft.color565(0, 180, 0));
  delay(350);
  tft.drawBitmap((TFT_WIDTH_SCREEN - 180) / 2, (TFT_HEIGHT_SCREEN - 80) / 2,
                 logovt180x80, 180, 80, TFT_GREEN);
  delay(150);
  tft.drawBitmap((TFT_WIDTH_SCREEN - 180) / 2, (TFT_HEIGHT_SCREEN - 80) / 2,
                 logovt180x80, 180, 80, tft.color565(0, 180, 0));

  sweepTone(15800, 20000, 200, 3);
  drawScanlines();
 // delay(100);

  // --- Переходим на спрайт-консоль ---
  tft.fillScreen(TFT_BLACK);
  drawScanlines();

#ifdef CYD2_4
  startupInit(clockFont, 1);
#endif

#ifdef CYD3_5
  startupInit(clockFont, 1);
#endif

  startupSetDelay(0);
  // startupSetColor(TFT_GREEN);

  delay(50);
#ifdef CYD3_5
  startupPrintln("=== RobCo Ind. PipBoy 3000 v3.5 ===");
#endif

#ifdef CYD2_4
  startupPrintln("=== RobCo Ind. PipBoy 3000 v2.4 ===");
#endif

  startupNewLine();
  startupPrintln("******* Start-up Self Check *******");

  startupNewLine();
  Wire.begin(RTC_SDA, RTC_SCL);
  //delay(50);

  rtcInit();
  //delay(50);
  eepromInit();
  //startupPrint(".");
  //delay(50);
  wifiInit();
  //startupPrintln(".");
  //delay(50);

  startupNewLine();
  startupSprite.pushSprite(0, 0);
  initI2C();
  clackBuzzer();

  startupPrint("RTC module ------------------ ");
  if (rtcFound)
    startupPrintln("OK");
  else {
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
  }
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  // startupSetColor(TFT_GREEN);

  clackBuzzer();

  startupPrint("BMP module ------------------ ");
  if (bmpInit()) {
    startupPrintln("OK");
    bmpCalibrateAltitude();
    bmpFound = true;
  } else {
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
    bmpFound = false;
  }
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  // startupSetColor(TFT_GREEN);

  clackBuzzer();

  startupPrint("GPS module ------------------ ");

  gpsInit();

  if (gpsCheckConnection()) {
    startupPrintln("OK");
    if (DEBUGFLAG) Serial.println("[GPS] Module connected: [OK]");
    GPS_Connected = true;
  } else {
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
    if (DEBUGFLAG) Serial.println("[GPS] Module connected: [ERROR]");
    GPS_Connected = false;
  }
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  // startupSetColor(TFT_GREEN);

  clackBuzzer();
  if (eepromFound) {
    startupPrintln("EEPROM module --------------- OK");
    startupNewLine();
    startupSprite.pushSprite(0, 0);

    startupPrint("BACKUP LOAD ----------------- ");
    if (LoadBackUpFromEPPR()) {
      startupPrintln("OK");
    } else {
      // startupSetColor(TFT_RED);
      startupPrintln("ERROR");
      SaveBackUpToEPPR();
    }
  } else {
    startupPrint("EEPROM module --------------- ");
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
    // startupSetColor(TFT_GREEN);
    startupNewLine();
    startupSprite.pushSprite(0, 0);
    startupPrint("BACKUP LOAD ----------------- ");
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
  }

  clackBuzzer();
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  // startupSetColor(TFT_GREEN);

  startupPrint("WEATHER backup module ------- ");
  if (weatherLoadFromEEPROM())
    startupPrintln("OK");
  else {
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
  }

  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  // startupSetColor(TFT_GREEN);
  startupPrintln("Wi-Fi module ---------------- OK");

  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  // startupSetColor(TFT_GREEN);
  startupPrintln("KEYBOARD module ------------- OK");

  radioInit();

  clackBuzzer();
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  startupPrint("SD card --------------------- ");
  if (sdCardInitialized)
    startupPrintln("OK");
  else {
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
    // startupSetColor(TFT_GREEN);
  }

  clackBuzzer();
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  startupPrint("MAP module ------------------ ");
  if (sdCardInitialized) {
    pipMaps = new MapsModule();
    pipMaps->begin();
    pipMaps->disableSprite();
    startupPrintln("OK");
  } else {
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
    // startupSetColor(TFT_GREEN);
  }

  clackBuzzer();
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  startupPrint("LASER module ---------------- ");
  if (tofSensor.isAvailable()) {
    laserModule = true;
    startupPrintln("OK");
  } else {
    // startupSetColor(TFT_RED);
    laserModule = false;
    startupPrintln("ERROR");
    // startupSetColor(TFT_GREEN);
  }

  radioStartTask();

  clackBuzzer();

  startupNewLine();
  startupPrint("PULSE SENSOR module  -------- ");
  if (pulseInit()) pulseSensFound = true;
  if (pulseCheckConnection()) pulseSensFound = true; else pulseSensFound = false;
  if (pulseSensFound) {
    startupPrintln("OK");
    pulseSensFound = true;
  } else {
    // startupSetColor(TFT_RED);
    startupPrintln("ERROR");
    pulseSensFound = false;
  }
  //startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  // startupSetColor(TFT_GREEN);

  // --- Тест RGB ---
  startupNewLine();
  startupSprite.pushSprite(0, 0);
  //delay(200);
  // startupSetColor(TFT_GREEN);
  startupPrint("RGB module ------------");
  delay(20);
  startupPrint("--");
  digitalWrite(LED_R, LOW); delay(200);
  clackBuzzer();
  digitalWrite(LED_R, HIGH);
  delay(10);

  startupPrint("--");
  digitalWrite(LED_G, LOW); delay(200);
  clackBuzzer();
  digitalWrite(LED_G, HIGH);
  delay(10);

  startupPrint("--");
  digitalWrite(LED_B, LOW); delay(200);
  clackBuzzer();
  digitalWrite(LED_B, HIGH);
  delay(10);
  startupPrintln(" OK");

  initKeyboard();
  //delay(100);
  startupNewLine();
  startupPrintln("***** Self check END *****");
  startupNewLine();
  delay(10);
  startupPrintln("Loading operating system....");
  delay(100);
  // Освобождаем спрайт — экономим RAM перед основным циклом
  startupRelease();
}


bool LoadBackUpFromEPPR()
{
BackUpDataGPS dataG;
BackUpDataWiFi dataW;
BackUpDataFolder dataF;
BackUpTimers dataTS;

bool ErrorB = false;
  // Очистка перед чтением
  memset(&dataG, 0, sizeof(BackUpDataGPS));
  if (!eepromReadSlot(1, (uint8_t*)&dataG)) {
    if (DEBUGFLAG) Serial.println("[EEPROM] read slot backup GPS ---- failed");
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
    if (DEBUGFLAG) Serial.println("[EEPROM] read slot backup wifi pass ---- failed");
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
    if (DEBUGFLAG) Serial.println("[EEPROM] read slot backup MP3 folder ---- failed");
    ErrorB = true;
  }
  else
  {  radioSDFolder = String(dataF.SDFolder);
  //StandartWiFiPass[31] = '\0';
  //StandartWiFiPass = String(dataW.WiFi_Pass);
  }


  memset(&dataTS, 0, sizeof(BackUpTimers));
  if (!eepromReadSlot(4, (uint8_t*)&dataTS)) {
    if (DEBUGFLAG) Serial.println("[EEPROM] read slot backup Timers ---- failed");
    ErrorB = true;
  }
  else
  {  
    char buf[9];
    
    memset(buf, 0, sizeof(buf));
    memcpy(buf,dataTS.T1Name, 8);
    buf[9] = '\0';
    T1S = String(buf);

    memset(buf, 0, sizeof(buf));
    memcpy(buf,dataTS.T2Name, 8);
    buf[9] = '\0';
    T2S = String(buf);

    memset(buf, 0, sizeof(buf));
    memcpy(buf,dataTS.T3Name, 8);
    buf[9] = '\0';
    T3S = String(buf);

    T1h = dataTS.T1hour;
    T2h = dataTS.T2hour;
    T3h = dataTS.T3hour;

    T1m = dataTS.T1min;
    T2m = dataTS.T2min;
    T3m = dataTS.T3min;
  }

  if (ErrorB)
  {
    if (DEBUGFLAG) Serial.printf("\n[EEPROM] read slot TOTAL backup --- FAILED:\n\nCurrent values:\nSLOT1:\nGPS: %s , %s\nSLOT2:\n WiFi Pass: %s\nSLOT3:\n MP3 folder:%s\nSLOT4:\n Time timers:\n  1:%s %d:%d\n  2:%s %d:%d\n  3:%s %d:%d\n\n", weatherLat, weatherLon, StandartWiFiPass, radioSDFolder, T1S, T1h, T1m, T2S, T2h, T2m, T3S, T3h, T3m);
    return false;
  }
  else
  {
  if (DEBUGFLAG) Serial.printf("\n[EEPROM] read slot backup --- OK:\nSLOT1:\n GPS: %s , %s\nSLOT2:\n WiFi Pass: %s\nSLOT3:\n MP3 folder:%s\nSLOT4:\n Time timers:\n %s %d:%d\n %s %d:%d\n %s %d:%d\n\n", weatherLat, weatherLon, StandartWiFiPass, radioSDFolder, T1S, T1h, T1m, T2S, T2h, T2m, T3S, T3h, T3m);
    return true;
  }
}

bool SaveBackUpToEPPR()
{
BackUpDataGPS dataG;
BackUpDataWiFi dataW;
BackUpDataFolder dataF;
BackUpTimers dataTS;
  // Очистка всей структуры перед использованием!
  // Weather settings
  memset(&dataG, 0, sizeof(BackUpDataGPS));

  strncpy(dataG.GPS_LAT, weatherLat.c_str(),  sizeof(dataG.GPS_LAT) - 1);
  dataG.GPS_LAT[sizeof(dataG.GPS_LAT)-1] = '\0';

  strncpy(dataG.GPS_LON, weatherLon.c_str(),  sizeof(dataG.GPS_LON) - 1);
  dataG.GPS_LON[sizeof(dataG.GPS_LON)-1] = '\0';

  if (weatherCelsius) dataG.icCels = 1; else dataG.icCels = 0;
  if (eepromWriteSlot(1, (uint8_t*)&dataG)) {
    if (DEBUGFLAG) Serial.printf("GPS saved to EEPROM: Lat: %s, Lon: %s, Celsius(1 - yes): %d\n", dataG.GPS_LAT ,dataG.GPS_LON, dataG.icCels);
  }
  else 
    return false;
/// WiFi pass
  memset(&dataW, 0, sizeof(BackUpDataWiFi));
  strncpy(dataW.WiFi_Pass, StandartWiFiPass, sizeof(dataW.WiFi_Pass) - 1);
  dataW.WiFi_Pass[sizeof(dataW.WiFi_Pass)-1] = '\0';

  if (eepromWriteSlot(2, (uint8_t*)&dataW)) {
    if (DEBUGFLAG) Serial.printf("Wifi pass saved to EEPROM: %s\n", dataW.WiFi_Pass);
  }
  else 
    return false;
// MP3 Folder
if (checkSDPath(radioSDFolder.c_str()))
{
  memset(&dataF, 0, sizeof(BackUpDataFolder));
  strncpy(dataF.SDFolder, radioSDFolder.c_str(), sizeof(dataF.SDFolder) - 1);
  dataF.SDFolder[sizeof(dataF.SDFolder)-1] = '\0';

  if (eepromWriteSlot(3, (uint8_t*)&dataF)) {
    if (DEBUGFLAG) Serial.printf("MP3 folder saved to EEPROM: %s\n", dataF.SDFolder);
  }
  else 
    return false;
} else
if (DEBUGFLAG) Serial.printf("Cannot check SD folder %s\n", dataF.SDFolder);

/// Timers
  memset(&dataTS, 0, sizeof(BackUpTimers));

  strncpy(dataTS.T1Name, T1S.c_str(),  sizeof(dataTS.T1Name));
  
  strncpy(dataTS.T2Name, T2S.c_str(),  sizeof(dataTS.T2Name));

  strncpy(dataTS.T3Name, T3S.c_str(),  sizeof(dataTS.T3Name));

  dataTS.T1hour = T1h;
  dataTS.T2hour = T2h;
  dataTS.T3hour = T3h;

  dataTS.T1min = T1m;
  dataTS.T2min = T2m;
  dataTS.T3min = T3m;

  if (eepromWriteSlot(4, (uint8_t*)&dataTS)) {
    if (DEBUGFLAG) Serial.printf("Timers saved to EEPROM:\n  %s %d:%d\n  %s %d:%d\n  %s %d:%d\n\n", dataTS.T1Name ,dataTS.T1hour, dataTS.T1min, dataTS.T2Name ,dataTS.T2hour, dataTS.T2min, dataTS.T3Name ,dataTS.T3hour, dataTS.T3min);
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
      clickBuzzer();
      digitalWrite(LED_G, HIGH);
      
      switch(currentScreen) {
        case 0: drawPipBoyScreen(); break;
        case 1: drawPipBoyScreen1(); break;
        case 2: drawPipBoyScreen2(); break;
        case 3: 
          //weatherForceUpdate(); 
          drawPipBoyScreen3(); 
          break;
        case 4: drawPipBoyScreen5(); break;
        case 5: drawPipBoyScreen4(); break;
      }
      
      drawTabButtons();
    }

    
    delay(150);
    return;
  }
  
  // Обработка по экранам
  if (currentScreen == 0) {
    vaultFrame = (vaultFrame + 1) % 16;
    drawVaultBoy(VBOYSTARTX, VBOYSTARTY, vaultFrame);
    clickBuzzer();
  }
  else if (currentScreen == 2) {
      handleRadioSetButtons(x, y);
  }
  else if (currentScreen == 4){
    handleButtonScreen5(x, y);
  }
  else if (currentScreen == TAB_COUNT - 1) {
        HandleButtonsScreen4(x, y);
  if (weatherSettingsActive) {
        handleWeatherSettingsTouch(x, y);
      } else if (radioSettingsActive) {
        handleRadioSettingsTouch(x, y);
      } else if (timeSettingsActive) {  
        HandleTimeSettings(x,y);
      } else
      {  
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
  
 tft.fillRect(cx, cy, 170, 170, TFT_BLACK);
  drawScanlinesButtons(cx, cy, 172, 170);
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(LVL_TEXT_SIZE);
  tft.setTextDatum(BC_DATUM);

  String lvlInfo = PERSON_NAME " Level " + String(frame + 1);
  int widthtext = tft.textWidth(String(lvlInfo + "S"));
  int heighttext = tft.fontHeight();

  tft.fillRect((TFT_WIDTH_SCREEN / 2) - (widthtext/2), TAB_Y - heighttext - 2, widthtext, heighttext, TFT_BLACK);
  drawScanlinesButtons((TFT_WIDTH_SCREEN / 2) - (widthtext/2),TAB_Y - heighttext - 2 , heighttext + 2, widthtext);
  
  tft.drawBitmap(cx, cy, specialBMP[frame], 170, 170, TFT_GREEN);
  tft.drawString(lvlInfo, TFT_WIDTH_SCREEN / 2, TAB_Y - 2);
  //if (DEBUGFLAG) Serial.printf("[GUI] heighttext = %d, start yrect = %d, ystring = %d, tabY = %d\n", heighttext , TAB_Y - heighttext*2, TAB_Y - heighttext, TAB_Y);
 
  updateLevel(frame + 1);
}



void drawPipBoyScreen() {
  if (DEBUGFLAG) Serial.println("[GUI] Open screen 0 - Main Screen");
  clickBuzzer();
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  
  // Top bar
  updateHPAP();

  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.drawRect(0, 0, TFT_WIDTH_SCREEN, 28, TFT_GREEN);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setCursor(12, 6);
  tft.print("STATS");
  /*tft.setTextSize(1);
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
  }*/


  UpdateLeftPanel();
  // Vault Boy
  drawVaultBoy(VBOYSTARTX, VBOYSTARTY, vaultFrame);
  
  UpdateRightPanel();

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
  int width = TFT_WIDTH_SCREEN;//tft.width();
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

void TimeSourceUpdate()
{
  // Индикатор источника
  if (rtcFound || ntpSynced || gpsTimeSynced)
  {
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(tft.color565(0, 100, 0));
    // Закрасить старое значение

    String src = (ntpSynced) ? "[NTP]" : 
                (gpsTimeSynced) ? "[GPS]" : "[RTC]";
    int tws = tft.textWidth(src);     
    int th = tft.fontHeight();        
    tft.fillRect(TFT_WIDTH_SCREEN - tws - 5, 10, tws + 5, th, TFT_BLACK); 
    drawScanlinesButtons(TFT_WIDTH_SCREEN - tws - 5, 10, th+2, tws + 5);
    tft.drawString(src, TFT_WIDTH_SCREEN - tws - 5, 10);
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
    tft.setTextDatum(TC_DATUM);
    char buffer[50];
    if (NOT_US_DATE) {
      sprintf(buffer, "%02d.%02d.%d", dd, mth, yr);
    } else {
      sprintf(buffer, "%02d/%02d/%d", mth, dd, yr);
    }
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(4);
    int h = tft.fontHeight();
    tft.fillRect(0, (digs[0]->Y() + digs[0]->Height()) + ((SCREEN_BOTTOM_Y - (digs[0]->Y() - digs[0]->Height())) / 2 + h/2), TFT_WIDTH_SCREEN, h, TFT_BLACK);
    drawScanlinesButtons(0, (digs[0]->Y() + digs[0]->Height()) + ((SCREEN_BOTTOM_Y - (digs[0]->Y() - digs[0]->Height())) / 2 + h/2), h, TFT_WIDTH_SCREEN);
    tft.drawString(buffer,TFT_WIDTH_SCREEN / 2,  (digs[0]->Y() + digs[0]->Height()) + ((SCREEN_BOTTOM_Y - (digs[0]->Y() - digs[0]->Height())) / 2 + h/2));

    int dow = weekday(local);
    String dayNames[] = {"", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    tft.setTextSize(3);
    //tft.fillRect(0,SCREEN_BOTTOM_Y - (h * 3) - 20, TFT_WIDTH_SCREEN, h, TFT_BLACK);

    tft.fillRect(0, (digs[0]->Y() + digs[0]->Height()) + ((SCREEN_BOTTOM_Y - (digs[0]->Y() - digs[0]->Height())) / 2 - h), TFT_WIDTH_SCREEN, h, TFT_BLACK);
    drawScanlinesButtons(0, (digs[0]->Y() + digs[0]->Height()) + ((SCREEN_BOTTOM_Y - (digs[0]->Y() - digs[0]->Height())) / 2 - h), h, TFT_WIDTH_SCREEN);
    tft.drawString(dayNames[dow], TFT_WIDTH_SCREEN / 2, (digs[0]->Y() + digs[0]->Height()) + ((SCREEN_BOTTOM_Y - (digs[0]->Y() - digs[0]->Height())) / 2 - h));
    prevDayCache = dd;
  }
}

void drawPipBoyScreen1() {

  if (DEBUGFLAG) Serial.println("[GUI] Open screen 1 : CLOCK");
  clickBuzzer();
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
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
    tft.drawString("CLOCK", TFT_WIDTH_SCREEN / 2, 10);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    
  tft.fillRect((TFT_WIDTH_SCREEN - TFT_HEIGHT_SCREEN) / 2 , 80, TFT_HEIGHT_SCREEN, 80, TFT_BLACK);
  tft.drawRect((TFT_WIDTH_SCREEN - TFT_HEIGHT_SCREEN) / 2, 80, TFT_HEIGHT_SCREEN, 80, TFT_GREEN);
    //tft.setTextColor(tft.color565(0, 50, 0));

    tft.setTextColor(TFT_RED);
    tft.drawString("NO TIME SYNC", TFT_WIDTH_SCREEN / 2, 110);
    tft.drawString("Connect WiFi", TFT_WIDTH_SCREEN / 2, 130);
    lastScreen = currentScreen;
    return;
  }
  
  time_t current = now();
  prevDisplay = current;
  //DrawDate();
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("CLOCK", TFT_WIDTH_SCREEN / 2, 10);
  DrawDate(prevDisplay);
  DrawColons();
  ParseDigits(prevDisplay);
  DrawDigitsAtOnce();
  DrawAmPm();
  //delay(1000);
  lastScreen = currentScreen;
}


void drawPipBoyScreen2() {
  if (DEBUGFLAG) Serial.println("[GUI] Open screen 2 : RADIO");
  clickBuzzer();
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
 tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("RADIO", TFT_WIDTH_SCREEN / 2, 10);

  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(TC_DATUM);
  
  //tft.setCursor(5, 25);
  tft.drawRect(6, 24, 30, 14, TFT_GREEN);
  String src = (radioPlaySource == 0) ? "SD" : (radioPlaySource == 1) ? "WiFi" : "Ext";
  //tft.print("Play: ");
  tft.drawString(src, 6 + 30 / 2, 20 + 14 / 2);

  //String VolumeInfo = "Volume: " + String(radioGetVolume());
 
  //tft.drawString(VolumeInfo, TFT_WIDTH_SCREEN / 2, 148);

  if (WiFi.status() != WL_CONNECTED && radioPlaySource == 1) 
  {
    if (radioIsPlaying())
      UpdateMetaData();
      //tft.drawString(radioGetStationName(), TFT_WIDTH_SCREEN / 2, 100);
    else
      tft.drawString("ConnectWiFi", TFT_WIDTH_SCREEN / 2, 100);
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
   // tft.drawString(radioGetStationName(), TFT_WIDTH_SCREEN / 2, 100);
    
  }

      //UpdateMetaData();

  drawRadioSetButtons();
  drawTabButtons();
  lastScreen = currentScreen;
}

void drawPipBoyScreen3() {
  if (DEBUGFLAG) Serial.println("[GUI] Open screen 3 : WEATHER");
  //clickBuzzer();
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("WEATHER", TFT_WIDTH_SCREEN / 2, 10);
  
  updateWeatherScreen();
  
  drawTabButtons();
 
  lastScreen = currentScreen;
}



// ======================= ЭКРАН 4: GENERAL =======================
void drawPipBoyScreen4() {

  if (DEBUGFLAG) Serial.println("[GUI] Open screen 4 : GENERAL (Setup)");
  clickBuzzer();
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
      clickBuzzer();
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
        timeSettingsActive = false;
        drawWiFiScreen();
        break;
      case 2:
        weatherSettingsActive = false;
        radioSettingsActive = true;
        timeSettingsActive = false;
        drawRadioSettings();
        break;
      case 3:
        weatherSettingsActive = false;
        radioSettingsActive = false;
        timeSettingsActive = true;
        drawTimeSettings();
        break; 
      case 4:
        weatherSettingsActive = true;
        radioSettingsActive = false;
        timeSettingsActive = false;
        drawWeatherSettings();
        break;
    }
    
    drawTabButtons();
  }
}

void drawWeatherSettings() {
  weatherSettingsActive = true;
  
  tft.fillRect(SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, TFT_BLACK);
  tft.drawRect(SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, TFT_GREEN);
  //tft.drawRect(SCREEN_X + 2, LIST_Y + 2, LIST_W - 4, LIST_H - 4, tft.color565(0, 100, 0));
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("WEATHER SETTINGS", SCREEN_CENTER, SCREEN_Y + 5);
  
  tft.setTextSize(1);
  int y = SCREEN_HEADER_Y;
  //tft.setCursor(SCREEN_X + 10, y + INPUT_FIELD_H / 2);
  tft.setTextDatum(MR_DATUM);
  tft.drawString("GPS LATITUDE:", SCREEN_X + 90, y + INPUT_FIELD_H / 2);
  tft.fillRect(SCREEN_X + 95, y, 110, INPUT_FIELD_H, TFT_BLACK);
  tft.drawRect(SCREEN_X + 95, y, 110, INPUT_FIELD_H, TFT_GREEN);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(weatherLat,SCREEN_X + 100, y + INPUT_FIELD_H / 2);
  y = y + INPUT_FIELD_H + 15;
  tft.setCursor(SCREEN_X + 10, y + INPUT_FIELD_H / 2);
  tft.setTextDatum(MR_DATUM);
  tft.drawString("GPS LONGITUDE:", SCREEN_X + 90, y + INPUT_FIELD_H / 2);
  tft.fillRect(SCREEN_X + 95, y, 110, INPUT_FIELD_H, TFT_BLACK);
  tft.drawRect(SCREEN_X + 95, y, 110, INPUT_FIELD_H, TFT_GREEN);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(weatherLon, SCREEN_X + 100, y + INPUT_FIELD_H / 2);
  
  
  
  int btnW = 60;
  int btnX = SCREEN_CENTER - btnW - 5;
  int btnH = BUTTON_H;
  y = y + INPUT_FIELD_H + 15;
  int btnY = y;
  
  //tft.setCursor(SCREEN_X + 10, btnY + btnH / 2);
  tft.setTextDatum(MR_DATUM);
  tft.drawString("UNITS:", btnX - 5, btnY + btnH/2);
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
  
  btnX = SCREEN_CENTER + 5;
  if (!weatherCelsius) {
    tft.fillRect(btnX, btnY, btnW, btnH, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(btnX, btnY, btnW, btnH, TFT_BLACK);
    tft.drawRect(btnX, btnY, btnW, btnH, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  tft.drawString("F", btnX + btnW/2, btnY + btnH/2);

  if (GPS_Connected) {
    tft.fillRect(SCREEN_CENTER - 60, btnY + btnH + 10, 120, btnH, TFT_BLACK);
    tft.drawRect(SCREEN_CENTER - 60, btnY + btnH + 10, 120, btnH, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("Insert GPS coords", SCREEN_CENTER, btnY + btnH + 10 + btnH/2);
  }
  
  tft.fillRect(SCREEN_CENTER - 65, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_BLACK);
  tft.drawRect(SCREEN_CENTER - 65, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_GREEN);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("SAVE", SCREEN_X + ((TFT_WIDTH_SCREEN - SCREEN_X - 5)/2 - 70/2), SCREEN_BOTTOM_Y - 5 - BUTTON_H + BUTTON_H / 2 );
  
  tft.fillRect(SCREEN_CENTER + 5, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_BLACK);
  tft.drawRect(SCREEN_CENTER + 5, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_GREEN);
  tft.drawString("CANCEL", SCREEN_X + ((TFT_WIDTH_SCREEN - SCREEN_X - 5)/2 + 70/2), SCREEN_BOTTOM_Y - 5 - BUTTON_H + BUTTON_H / 2);
  
  tft.setTextColor(tft.color565(0, 100, 0));
  tft.setTextDatum(TC_DATUM);
  if (weatherLastUpdate())
  {
    time_t dateUpW = weatherLastUpdate();
    time_t local = myTZ.toLocal(dateUpW, &tcr);
    String stringw = "Last update: " + pad2(day(local)) + "." + pad2(month(local)) + "." + String(year(local)) + " " + pad2(hour(local)) + ":" + pad2(minute(local));
    tft.drawString(stringw, SCREEN_CENTER , SCREEN_Y + SCREEN_H - BUTTON_H - 10 - 8);
  }
}

void handleWeatherSettingsTouch(uint16_t x, uint16_t y) {
  if (x < SCREEN_X || x > TFT_WIDTH_SCREEN - 5 || y < SCREEN_HEADER_Y || y > SCREEN_BOTTOM_Y) {
    return;
  }
  
  if (x >= SCREEN_X + 95 && x <= SCREEN_X + 215 && y >= SCREEN_HEADER_Y && y <= SCREEN_HEADER_Y + INPUT_FIELD_H) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    editlat = true;
    showKeyboard(weatherLat.c_str());
    clickBuzzer();
   // weatherLat = String(getKeyboardInput());
    //drawWeatherSettings();
    return;
  }
  
  if (x >= SCREEN_X + 95 && x <= SCREEN_X + 215 && y >= SCREEN_HEADER_Y + INPUT_FIELD_H + 15 && y <= SCREEN_HEADER_Y + INPUT_FIELD_H * 2 + 15 ) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    editlon = true;
    clickBuzzer();
    showKeyboard(weatherLon.c_str());
    //weatherLon = String(getKeyboardInput());
    //drawWeatherSettings();
    return;
  }

  int btnW = 60;

//celsus button
  if (x >= SCREEN_CENTER - btnW - 5 && x <= SCREEN_CENTER - 5 && y >= SCREEN_HEADER_Y + INPUT_FIELD_H * 2 + 15 * 2 && y <= SCREEN_HEADER_Y + INPUT_FIELD_H * 2 + 15 * 2 + BUTTON_H) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    weatherCelsius = true;
    clickBuzzer();
    drawWeatherSettings();
    return;
  }
  // farengheit button
  if (x >= SCREEN_CENTER + 5 && x <= SCREEN_CENTER + btnW + 5 && y >= SCREEN_HEADER_Y + INPUT_FIELD_H * 2 + 15 * 2 && y <= SCREEN_HEADER_Y + INPUT_FIELD_H * 2 + 15 * 2 + BUTTON_H) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    weatherCelsius = false;
    clickBuzzer();
    drawWeatherSettings();
    return;
  }
  // gps get
  if (x >= SCREEN_CENTER - 60 && x <= SCREEN_CENTER + 60 && y >= SCREEN_HEADER_Y + INPUT_FIELD_H * 2 + 15 * 2 + BUTTON_H + 10 && y <= SCREEN_HEADER_Y + INPUT_FIELD_H * 2 + 15 * 2 + 10 + BUTTON_H*2) {
  if (gpsHasFix()) {
    digitalWrite(LED_B, LOW);
    tft.fillRect(SCREEN_CENTER - 60, SCREEN_HEADER_Y + INPUT_FIELD_H * 2 + 15 * 2 + BUTTON_H + 10 , 120, BUTTON_H, TFT_GREEN);
    delay(30);
    digitalWrite(LED_B, HIGH);
    weatherLat = String(gpsGetLat(), 6);
    weatherLon = String(gpsGetLon(), 6);
    clickBuzzer();
    drawWeatherSettings();
    return;
  }
  }

  // OK
  if (x >= SCREEN_CENTER - 65 && x <= SCREEN_CENTER - 5 && y >= SCREEN_BOTTOM_Y - 5 - BUTTON_H && y <= SCREEN_BOTTOM_Y - 5) {
    digitalWrite(LED_G, LOW);
    delay(30);
    digitalWrite(LED_G, HIGH);
    weatherSettingsActive = false;
    clickBuzzer();
    SaveBackUpToEPPR(); 
    weatherForceUpdate();
    drawPipBoyScreen4();
    return;
  }
  
  // CANCEL
  if (x >= SCREEN_CENTER + 5 && x <= SCREEN_CENTER + 65 && y >= SCREEN_BOTTOM_Y - 5 - BUTTON_H && y <= SCREEN_BOTTOM_Y - 5) {
    digitalWrite(LED_R, LOW);
    delay(30);
    digitalWrite(LED_R, HIGH);
    clickBuzzer();
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
    if (DEBUGFLAG) Serial.println("[checkSDPath] SD not initialized");
    return false;
  }
  if (!path || strlen(path) < 1) {
    if (DEBUGFLAG) Serial.println("[checkSDPath] Path too short");
    return false;
  }

  String p = String(path);
  
  // Сначала проверяем формат пути
  if (!isValidPath(p)) {
    if (DEBUGFLAG) Serial.println("[checkSDPath] Invalid path format");
    return false;
  }

  // Защита от повторного вызова
  static bool checking = false;
  if (checking) return false;
  checking = true;

  if (DEBUGFLAG) Serial.printf("[checkSDPath] Testing: '%s'\n", path);

  //digitalWrite(5, LOW);
 // delay(10);

  File dir = SD.open(path);
  bool ok = (dir && dir.isDirectory());
  if (dir) dir.close();

  //digitalWrite(5, HIGH);
  checking = false;

  if (DEBUGFLAG) Serial.printf("[checkSDPath] Result: %s\n", ok ? "VALID DIRECTORY" : "NOT FOUND / NOT DIRECTORY");
  return ok;
}

void drawTimeSettings()
{
  timeSettingsActive = true;
  tft.fillRect(SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, TFT_BLACK);
  tft.drawRect(SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, TFT_GREEN);

  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("TIMER SETTINGS", SCREEN_CENTER, 15);
  tft.setTextSize(1);

      tft.setCursor(SCREEN_X + 10, 110);
      //int timersFH = tft.fontHeight();
      int fieldX = SCREEN_X + 10, fieldY = SCREEN_HEADER_Y, fieldW = 100, fieldH = INPUT_FIELD_H, feldPad = 5;

      for (int i = 1; i <= 3; i++)
      {
        tft.fillRect(fieldX, fieldY + (fieldH * (i-1)) + feldPad*(i-1), fieldW, fieldH, TFT_BLACK);
        tft.drawRect(fieldX, fieldY + (fieldH * (i-1)) + feldPad*(i-1), fieldW, fieldH, TFT_GREEN);
        tft.setCursor(fieldX + 5, fieldY + (fieldH * (i-1)) + feldPad*(i-1) +5);
         switch(i) {
            case 1: tft.print(T1S); break;
            case 2: tft.print(T2S); break;
            case 3: tft.print(T3S); break;
         }
        tft.setCursor(fieldX + fieldW + feldPad + 10, fieldY + (fieldH * (i-1)) + feldPad*(i-1) + 5);
        tft.print("Time:");
        tft.fillRect(fieldX + 40 + fieldW + feldPad, fieldY + (fieldH * (i-1)) + feldPad*(i-1), 25, fieldH, TFT_BLACK);
        tft.drawRect(fieldX + 40 + fieldW + feldPad, fieldY + (fieldH * (i-1)) + feldPad*(i-1), 25, fieldH, TFT_GREEN);
        tft.setCursor(fieldX + 40 + fieldW + feldPad + 5, fieldY + (fieldH * (i-1)) + feldPad*(i-1) + 5);
        switch(i) {
            case 1: tft.printf("%02u", T1h); break;
            case 2: tft.printf("%02u",T2h); break;
            case 3: tft.printf("%02u",T3h); break;
         }
        tft.fillRect(fieldX + 40 + fieldW + feldPad*2 + 25, fieldY + (fieldH * (i-1)) + feldPad*(i-1), 25, fieldH, TFT_BLACK);
        tft.drawRect(fieldX + 40 + fieldW + feldPad*2 + 25, fieldY + (fieldH * (i-1)) + feldPad*(i-1), 25, fieldH, TFT_GREEN);
        tft.setCursor(fieldX + 40 + fieldW + feldPad*2 + 25 + 5, fieldY + (fieldH * (i-1)) + feldPad*(i-1) + 5);
        switch(i) {
            case 1: tft.printf("%02u",T1m); break;
            case 2: tft.printf("%02u",T2m); break;
            case 3: tft.printf("%02u",T3m); break;
         }
      }


  // SAVE / CANCEL
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN);

  tft.fillRect(SCREEN_CENTER - 65, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_BLACK);
  tft.drawRect(SCREEN_CENTER - 65, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_GREEN);
  tft.drawString("SAVE", SCREEN_X + ((SCREEN_W)/2 - 35), SCREEN_BOTTOM_Y - 5 - BUTTON_H/2);

  tft.fillRect(SCREEN_CENTER + 5, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_BLACK);
  tft.drawRect(SCREEN_CENTER + 5, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_GREEN);
  tft.drawString("CANCEL", SCREEN_X + ((SCREEN_W)/2 + 35), SCREEN_BOTTOM_Y - 5 - BUTTON_H/2);
}


void HandleTimeSettings(uint16_t x, uint16_t y)
{

  if (x < SCREEN_X || x > SCREEN_W + SCREEN_X || y < SCREEN_Y || y > SCREEN_Y + SCREEN_H) return;
  clickBuzzer();
  if (timeSettingsActive)
  {
     int fieldX = SCREEN_X + 10, fieldY = SCREEN_HEADER_Y, fieldW = 100, fieldH = INPUT_FIELD_H, feldPad = 5;
      //if (DEBUGFLAG)  Serial.print("Touch in screen. Field value:");
   for (int i = 1; i <= 3; i++)
      {
          if (x >= fieldX && x <= fieldX + fieldW && y >= fieldY + (fieldH * (i-1)) + feldPad*(i-1) && y <= fieldY + (fieldH * (i-1)) + feldPad*(i-1) + fieldH) {
          digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
          
          switch(i) {
            case 1: EditFieldFlag = "T1S"; showKeyboard(T1S.c_str()); break; 
            case 2: EditFieldFlag = "T2S"; showKeyboard(T2S.c_str()); break; 
            case 3: EditFieldFlag = "T3S"; showKeyboard(T3S.c_str()); break; 
            }
          return;
          }

          if (x >= fieldX + 40 + fieldW + feldPad && x <= fieldX + 40 + fieldW + feldPad + 25 && y >= fieldY + (fieldH * (i-1)) + feldPad*(i-1) && y <= fieldY + (fieldH * (i-1)) + feldPad*(i-1) + fieldH) {
          digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);         
          switch(i) {
            case 1: EditFieldFlag = "T1h"; showKeyboard(String(T1h).c_str()); break; 
            case 2: EditFieldFlag = "T2h"; showKeyboard(String(T2h).c_str()); break;
            case 3: EditFieldFlag = "T3h"; showKeyboard(String(T3h).c_str()); break; 
            }
          return;
          }

          if (x >= fieldX + 40 + fieldW + feldPad * 2 + 25 && x <= fieldX + 40 + fieldW + feldPad * 2 + 55 && y >= fieldY + (fieldH * (i-1)) + feldPad*(i-1) && y <= fieldY + (fieldH * (i-1)) + feldPad*(i-1) + fieldH) {
          digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
          switch(i) {
            case 1: EditFieldFlag = "T1m"; showKeyboard(String(T1m).c_str()); break; 
            case 2: EditFieldFlag = "T2m"; showKeyboard(String(T2m).c_str()); break; 
            case 3: EditFieldFlag = "T3m"; showKeyboard(String(T3m).c_str()); break;
            }
          return;
          }
    
      }

      // SAVE

  if (x >= SCREEN_CENTER - 65 && x <= SCREEN_CENTER - 5 && y >= SCREEN_BOTTOM_Y - 5 - BUTTON_H && y <= SCREEN_BOTTOM_Y - 5) {
    digitalWrite(LED_G, LOW); delay(30); digitalWrite(LED_G, HIGH);
    timeSettingsActive = false;
      SaveBackUpToEPPR();
      //loadSDPlaylist();   // вызов из radio_module
    drawPipBoyScreen4();
    return;
  }

  // CANCEL
  if (x >= SCREEN_CENTER + 5 && x <= SCREEN_CENTER + 65 && y >= SCREEN_BOTTOM_Y - 5 - BUTTON_H && y <= SCREEN_BOTTOM_Y - 5) {
    digitalWrite(LED_R, LOW); delay(30); digitalWrite(LED_R, HIGH);
    timeSettingsActive = false;
    drawPipBoyScreen4();
    return;
  }
  

   if (DEBUGFLAG) Serial.println("");
  }
}

void drawRadioSettings() {
  radioSettingsActive = true;
  tft.fillRect(SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, TFT_BLACK);
  tft.drawRect(SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, TFT_GREEN);

  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("RADIO SETTINGS", SCREEN_CENTER, 15);

  tft.setTextSize(1);
  //tft.setCursor(SCREEN_X + 10, 45);
  tft.drawString("Select mp3 radio:", SCREEN_CENTER, SCREEN_HEADER_Y );

  int btnW = 60, btnH = BUTTON_H, btnY = SCREEN_HEADER_Y + 20;
  int startX = SCREEN_CENTER - (btnW * 3 + 10 * 2) / 2;

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
    tft.setTextColor(TFT_GREEN);
    if (!sdCardInitialized)
    {
      tft.drawString("Insert SD card",SCREEN_CENTER, SCREEN_HEADER_Y + BUTTON_H + 45);
    } else
    {
      tft.setCursor(SCREEN_X + 10, SCREEN_HEADER_Y + BUTTON_H + INPUT_FIELD_H/2 + 15);
      tft.print("Folder:");
      int fieldX = SCREEN_X + 70, fieldY = SCREEN_HEADER_Y + BUTTON_H + INPUT_FIELD_H/2 + 15, fieldW = 150, fieldH = INPUT_FIELD_H;
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

  tft.fillRect(SCREEN_CENTER - 65, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_BLACK);
  tft.drawRect(SCREEN_CENTER - 65, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_GREEN);
  tft.drawString("SAVE", SCREEN_X + ((SCREEN_W)/2 - 35), SCREEN_BOTTOM_Y - 5 - BUTTON_H/2);

  tft.fillRect(SCREEN_CENTER + 5, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_BLACK);
  tft.drawRect(SCREEN_CENTER + 5, SCREEN_BOTTOM_Y - 5 - BUTTON_H, 60, BUTTON_H, TFT_GREEN);
  tft.drawString("CANCEL", SCREEN_X + ((SCREEN_W)/2 + 35), SCREEN_BOTTOM_Y - 5 - BUTTON_H/2);
}

void handleRadioSettingsTouch(uint16_t x, uint16_t y) {
  if (x < SCREEN_X || x > (SCREEN_W + SCREEN_X) || y < SCREEN_Y || y > SCREEN_Y + SCREEN_H) return;

  int btnY = SCREEN_HEADER_Y + 20, btnH = BUTTON_H, btnW = 60;
   int startX = SCREEN_CENTER - (btnW * 3 + 10 * 2) / 2;

  // SD button
  if (y >= btnY && y <= btnY + btnH && x >= startX && x <= startX + btnW) {
    if (radioIsPlaying()) radioPause(); 
    digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
    clickBuzzer();
    radioPlaySource = 0; drawRadioSettings(); return;
  }
  startX += btnW + 10;

  // WiFi button
  if (y >= btnY && y <= btnY + btnH && x >= startX && x <= startX + btnW) {
    if (radioIsPlaying()) radioPause(); 
    digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
    clickBuzzer();
    radioPlaySource = 1; drawRadioSettings(); return;
  }
  startX += btnW + 10;

  // Ext button
  if (y >= btnY && y <= btnY + btnH && x >= startX && x <= startX + btnW) {
    if (radioIsPlaying()) radioPause();
    digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
    clickBuzzer();
    radioPlaySource = 2; drawRadioSettings(); return;
  }

  // Поле Folder (только SD)
  if (radioPlaySource == 0 && sdCardInitialized &&  x >= SCREEN_X + 70 && x <= SCREEN_X + 220 && y >= SCREEN_HEADER_Y + BUTTON_H + INPUT_FIELD_H/2 + 15 && y <= SCREEN_HEADER_Y + BUTTON_H + INPUT_FIELD_H/2 + 15 + INPUT_FIELD_H) {
    digitalWrite(LED_B, LOW); delay(30); digitalWrite(LED_B, HIGH);
    clickBuzzer();
    showKeyboard(radioSDFolder.c_str());
    return;
  }


  // SAVE
  if (x >= SCREEN_CENTER - 65 && x <= SCREEN_CENTER - 5 && y >= SCREEN_BOTTOM_Y - 5 - BUTTON_H && y <= SCREEN_BOTTOM_Y - 5) {
     digitalWrite(LED_G, LOW); delay(30); digitalWrite(LED_G, HIGH);
    radioSettingsActive = false;
    if (radioPlaySource == 0 && checkSDPath(radioSDFolder.c_str())) {
      SaveBackUpToEPPR();
      //loadSDPlaylist();   // вызов из radio_module
    }
    clickBuzzer();
    drawPipBoyScreen4();
    return;
  }

  // CANCEL
   if (x >= SCREEN_CENTER + 5 && x <= SCREEN_CENTER + 65 && y >= SCREEN_BOTTOM_Y - 5 - BUTTON_H && y <= SCREEN_BOTTOM_Y - 5) {
    digitalWrite(LED_R, LOW); delay(30); digitalWrite(LED_R, HIGH);
    radioSettingsActive = false;
    clickBuzzer();
    drawPipBoyScreen4();
    return;
  }
}

void handleRadioSetButtons(uint16_t x, uint16_t y) {
  int xp = (TFT_WIDTH_SCREEN / 6) - 2;
  int yp = TAB_Y - TAB_H - 5;

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
        clickBuzzer();
        switch(i) {
        case 0: {
                  //tft.fillRect(20, 80, 300, 40, TFT_BLACK);
                  //drawScanlinesButtons(20, 78, 82, 300);
                  //tft.drawString(radioGetStationName(), TFT_WIDTH_SCREEN / 2, 100);
                  if (DEBUGFLAG) Serial.print("[RADIO SCREEN] Play button \n");
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
                  if (DEBUGFLAG) Serial.print("[RADIO SCREEN] Pause button \n");
                  radioPause(); 
                  digitalWrite(LED_R, LOW);
                  delay(30);
                  digitalWrite(LED_R, HIGH);
                  break;
                }
        case 2: {
                  if (DEBUGFLAG) Serial.print("[RADIO SCREEN] Prev button \n");
                  indicateRGB = false; 
                  if (radioPlaySource == 0)
                    if (radioPrevTrack()) indicateRGB = true; 
                  if (radioPlaySource == 1)
                    if (radioPrevStation()) indicateRGB = true; 
                  //tft.fillRect(20, 80, 300, 40, TFT_BLACK);
                  //drawScanlinesButtons(20, 78, 82, 300);
                  //tft.drawString(radioGetStationName(), TFT_WIDTH_SCREEN / 2, 100);         
                  RadStationInd = radioGetStationIndex();  
                  break;
                }
        case 3: {
                  if (DEBUGFLAG) Serial.print("[RADIO SCREEN] Next button \n");
                  indicateRGB = false; 
                  if (radioPlaySource == 0)
                    if (radioNextTrack()) indicateRGB = true;
                  if (radioPlaySource == 1)
                    if (radioNextStation()) indicateRGB = true; 
                  //tft.fillRect(20, 80, 300, 40, TFT_BLACK);
                  //drawScanlinesButtons(20, 78, 82, 300);
                  //tft.drawString(radioGetStationName(), TFT_WIDTH_SCREEN / 2, 100);  
                  RadStationInd = radioGetStationIndex(); 
                  break;
                }
        case 4: {
                  if (DEBUGFLAG) Serial.print("[RADIO SCREEN] Vol - button \n");
                  RadVolume -= 10;
                  if (RadVolume < 10) RadVolume = 10;
                  radioSetVolume(RadVolume); 
                  break;
                }
        case 5: {
                  if (DEBUGFLAG) Serial.print("[RADIO SCREEN] Vol + button \n");
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
    tft.setTextSize(TAB_TEXT_SIZE);
    int x = RADIO_B_X;
    int y = RADIO_B_Y;
    for (int i = 0; i < 6; i++) {
      tft.fillRect(x * i + 5, y, x, RADIO_B_H, TFT_BLACK);
      drawScanlinesButtons(x * i + 5, y, RADIO_B_H, (TFT_WIDTH_SCREEN / 6));
      tft.drawRect(x * i + 5, y, x, RADIO_B_H, TFT_GREEN);
      switch(i) {
        case 0: tft.drawString("Play", x * i + 5 + x/2, y + RADIO_B_H/2); break;
        case 1: tft.drawString("Pause", x * i + 5 + x/2, y + RADIO_B_H/2); break;
        case 2: tft.drawString("Prev", x * i + 5 + x/2, y + RADIO_B_H/2); break;
        case 3: tft.drawString("Next", x * i + 5 + x/2, y + RADIO_B_H/2); break;
        case 4: tft.drawString("Vol -", x * i + 5 + x/2, y + RADIO_B_H/2); break;
        case 5: tft.drawString("Vol +", x * i + 5 + x/2, y + RADIO_B_H/2); break;
      }
      
    }
}


// ========== Экран карты ==========
void drawPipBoyScreen5()
{
    // --- Основная отрисовка ---
    tft.fillScreen(TFT_BLACK);
    
    // Заголовок экрана — Top-Center
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.drawString("WORLD MAP", TFT_WIDTH_SCREEN / 2, 8);

   
    bool gpsActive = false;
    
    if (GPS_Connected && gpsHasFix()) {
        gpsLat= gpsGetLat();
        gpsLon= gpsGetLon();
        gpsActive = true;
    }
    else
    {
        gpsLat= atof(weatherLat.c_str());
        gpsLon= atof(weatherLon.c_str());
    }
    
    uint8_t have = pipMaps->cachedCount(gpsLat, gpsLon, 16);
    
    // --- Экран загрузки ---
    
    if (have < 9 && WiFi.status() == WL_CONNECTED && GPS_Connected) {
        tft.fillScreen(TFT_BLACK);
        
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_GREEN);
        tft.setTextSize(2);
        tft.drawString("SYNCING MAP DATA...",  TFT_WIDTH_SCREEN / 2, SCREEN_HEADER_Y + SCREEN_CONTENT_H/2 - 20);
        
        // Рамка прогресс-бара
        uint16_t pbW = ( TFT_WIDTH_SCREEN / 2);
        uint16_t pbH = 20;
        uint16_t pbX = ( TFT_WIDTH_SCREEN / 2) - pbW/2;
        uint16_t pbY = SCREEN_HEADER_Y + SCREEN_CONTENT_H/2 + 10;
        tft.drawRect(pbX, pbY, pbW, pbH, TFT_GREEN);
        
       // pipMaps->ensureTiles(gpslat, gpslon, 16);
        pipMaps->ensureTiles(gpsLat,gpsLon,pipMaps->getZoom() , [&](uint8_t p) {
            tft.fillRect(pbX + 2, pbY + 2, (p * (pbW - 4)) / 100, pbH - 4, TFT_GREEN);
        });
        /*while (pipMaps->isLoading()) {
            uint8_t p = pipMaps->getProgress();
            tft.fillRect(pbX + 2, pbY + 2, (p * (pbW - 4)) / 100, pbH - 4, TFT_GREEN);
            delay(50);
        }*/
        delay(200);
    }
    

    
    //pipMaps->drawMap(gpslat, gpslon);
    // Рисуем GUI один раз: меню, заголовок "WORLD MAP", табы
   // drawGuiFrame();  

    // Создаём спрайт (один раз при входе на экран)
    pipMaps->initSprite();

    // Обновляем карту — ничего кроме области MAP_START_X/Y не мелькает
    pipMaps->redrawMap(gpsLat, gpsLon);
    
        // Заголовок экрана — Top-Center
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.drawString("WORLD MAP", TFT_WIDTH_SCREEN / 2, 8);

      
        tft.setTextDatum(TC_DATUM);
        tft.setTextSize(2);
        if (pipMaps->getZoom() == MAP_ZOOM_OUT)
        {
          GPSZoomOut = true;
          tft.setTextColor(TFT_GREEN);
          tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_BLACK);
          tft.drawRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.drawString("+", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13);
        } else
        {
          GPSZoomOut = false;
          tft.setTextColor(TFT_BLACK);
          tft.drawRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.drawString("+", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13);  
        }

        if (laserModule)
        {
          laserActive = false;
          if (!laserActive)
          {
            tft.setTextDatum(TC_DATUM);
            tft.setTextSize(2);
            tft.setTextColor(TFT_GREEN);
            tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y + 50, 42, 42, TFT_BLACK);
            tft.drawRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y + 50, 42, 42, TFT_GREEN);
            tft.drawString("*->", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13 + 50);      
            
          }
        }

    // GPS статус — Bottom-Left над табами
      UpdateMapInfoPanel();
        
        //return;
    
    
  drawTabButtons();
 
  lastScreen = currentScreen;

}

void handleButtonScreen5(uint16_t x, uint16_t y)
{
  if (x < 5|| x > TFT_WIDTH_SCREEN - 5 || y > SCREEN_BOTTOM_Y) {
    return;
  }
  
  if (x >= TFT_WIDTH_SCREEN - 5 - 45 && x <= TFT_WIDTH_SCREEN - 5 && y >= MAP_START_Y && y <= MAP_START_Y + 42) {
    digitalWrite(LED_G, LOW);
    delay(30);
    digitalWrite(LED_G, HIGH);
    clickBuzzer();
    pipMaps->toggleZoom();

   // Обновляем кнопку (цвет/текст)
      
        tft.setTextDatum(TC_DATUM);
        tft.setTextSize(2);
        if (pipMaps->getZoom() == MAP_ZOOM_OUT)
        {
          GPSZoomOut = true;
          tft.setTextColor(TFT_GREEN);
          tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_BLACK);
          tft.drawRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.drawString("+", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13);
        } else
        {
          GPSZoomOut = false;
          tft.setTextColor(TFT_BLACK);
          tft.drawRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y, 42, 42, TFT_GREEN);
          tft.drawString("+", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13);  
        }
       
    // Загружаем тайлы нового zoom (если нет на SD — скачает)
        uint16_t pbW = ( TFT_WIDTH_SCREEN / 2);
        uint16_t pbH = 8;
        uint16_t pbX = ( TFT_WIDTH_SCREEN / 2) - pbW/2;
        uint16_t pbY = MAP_START_Y + 8;
        //статус подгрузки карты
        tft.fillRect(pbX, pbY, pbW, pbH, TFT_BLACK);
        pipMaps->ensureTiles(gpsLat,gpsLon,pipMaps->getZoom() , [&](uint8_t p) {
            tft.fillRect(pbX + 2, pbY + 2, (p * (pbW - 4)) / 100, pbH - 4, TFT_GREEN);
        });

        // Перерисовываем карту в новом масштабе
    pipMaps->redrawMap(gpsLat, gpsLon);

    return;
  }

if (x >= TFT_WIDTH_SCREEN - 5 - 45 && x <= TFT_WIDTH_SCREEN - 5 && y >= MAP_START_Y+ 50 && y <= MAP_START_Y + 42+ 50 && laserModule) {
    digitalWrite(LED_G, LOW);
    delay(30);
    digitalWrite(LED_G, HIGH);
    clickBuzzer();
    laserActive = !laserActive;
    if (laserModule)
    {
          tft.setTextDatum(TC_DATUM);
          tft.setTextSize(2);
        if (laserActive)
        {
          tft.setTextColor(TFT_BLACK);
          tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y + 50, 42, 42, TFT_GREEN);
          tft.drawString("*->", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13 + 50);   
        }
        else
        {
            tft.setTextColor(TFT_GREEN);
            tft.fillRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y + 50, 42, 42, TFT_BLACK);
            tft.drawRect(TFT_WIDTH_SCREEN - 5 - 45 , MAP_START_Y + 50, 42, 42, TFT_GREEN);
            tft.drawString("*->", TFT_WIDTH_SCREEN - 5 - 22, MAP_START_Y + 13 + 50);
            tft.fillRect(TFT_WIDTH_SCREEN - 80 , 5, 80, 10, TFT_BLACK);
        }
    }
    }

}


void UpdateMapInfoPanel()
{
    
    bool gpsActive = false;
    
    if (GPS_Connected && gpsHasFix()) {
        gpsLat= gpsGetLat();
        gpsLon= gpsGetLon();
        gpsActive = true;
    }else
    {
      gpsLat= atof(weatherLat.c_str());
      gpsLon= atof(weatherLon.c_str());
    }
        uint8_t have = pipMaps->cachedCount(gpsLat, gpsLon, 16);
        tft.setTextSize(1);
        tft.fillRect(5 , 0, 90, 32, TFT_BLACK);
        tft.drawRect(5,  0, 90, 32, TFT_GREEN);
        
        tft.setTextDatum(TL_DATUM);
        if (have < 3)
        {
            tft.setTextColor(TFT_BROWN);
            tft.drawString("Cashed: NO", 9,  2);
        }else
        {
            tft.setTextColor(tft.color565(0, 180, 0));
            String str = "Cashed: " + String(have);
            tft.drawString(str, 9,  2);
        }

        if (gpsActive && GPS_Connected) 
          tft.setTextColor(TFT_GREEN);
        else
          tft.setTextColor(TFT_YELLOW);
          String gpscoords = String(gpsLat, 3) + " " + String(gpsLon, 3);
          tft.drawString(gpscoords, 9,  12);
        

        if (WiFi.status() != WL_CONNECTED)
        {
          tft.setTextColor(TFT_GREEN);
          tft.drawString("WiFi OFF", 9,  22);
        }
        tft.setTextDatum(BL_DATUM);
        tft.setTextSize(1);
            String gpsTstr = " ";
            
                gpsTstr = "                 ";
            tft.setCursor(5,  TAB_Y - 12);
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
        if (gpsActive && GPS_Connected) {     
            tft.print("[GPS]: <LIVE> | SATS:" + String(gpsGetSats()) + gpsTstr + " SPEED: " + String(gpsGetSpeedKmph())) ;
        } else {
            tft.print("[GPS]: <OFFLINE> " + gpsTstr );
        }
        tft.setCursor(MAP_START_X + 3, MAP_START_Y + 3);
        tft.setTextColor(TFT_BLACK, tft.color565(0, 190, 0));
        tft.print("Openstreetmap.org");
        tft.setTextColor(TFT_GREEN);


        
}

void drawTabButtons() {
  drawStatusButton(currentScreen == 0);
  drawSpecialButton(currentScreen == 1);
  drawSkillsButton(currentScreen == 2);
  drawWeatherButton(currentScreen == 3);
  drawMapButton(currentScreen == 4);
  drawGeneralButton(currentScreen == 5);
}

void drawMapButton(bool active) {
  int x =  4 * TAB_W;;
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
  tft.setTextSize(TAB_TEXT_SIZE);
  tft.drawString("MAP", x + TAB_W/2, TAB_Y + TAB_H/2);
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
  tft.setTextSize(TAB_TEXT_SIZE);
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
  int x = 5 * TAB_W;
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

void updateLevel(int8_t frame)
{
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.fillRect(80, 8, 45, 10, TFT_BLACK);
  drawScanlinesButtons(80, 6, 14, 45);
  tft.setCursor(80, 10);
  tft.printf("LVL %d", frame);
}

void updateHPAP() {

  if (pulseSensFound)
  {
    if (pulseSensorUpdate())
      psActive = true;
    else 
      psActive = false;
  } 

  if (!psActive)
  {
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
  }
    //if (DEBUGFLAG) Serial.printf("HP %d/%d AP%d/%d, curr minutes:%d\n", currentHP, hpMax, currentAP, apMax, minutes);

    if (currentScreen == 0)
    {
      tft.fillRect(122, 8, TFT_WIDTH_SCREEN - 60 - 3 - 122, 10, TFT_BLACK);
      drawScanlinesButtons(122, 5, 14, TFT_WIDTH_SCREEN - 3 - 122 - 60);
      tft.setTextColor(TFT_GREEN);
      tft.setTextSize(1);
      tft.setTextWrap(false);
      tft.setTextDatum(MC_DATUM);
      tft.setCursor(122, 10);
      if (psActive)
      {
        tft.printf("BPM %d/%d\n", currentHP, hpMax);
        tft.setCursor(200, 10);
        tft.printf("SPO2 %d/%d", currentAP, 100);
        //tft.setCursor(287, 10);
        
        //tft.print("PULSE ACTIVE");
      }
      else
      {
        tft.printf("HP %d/%d\n", currentHP, hpMax);
        tft.setCursor(200, 10);
        tft.printf("AP %d/%d", currentAP, apMax);
      }
     
    }
  
}


uint8_t minutesUntil(uint8_t targetH, uint8_t targetM) {
    time_t t = now();
    time_t local = myTZ.toLocal(t, &tcr);
    int h = hour(local);
    int m = minute(local);
    
    uint16_t current = h * 60 + m;
    uint16_t target = (targetH % 24) * 60 + (targetM % 60);
    
    if (target <= current) target += 1440;
    
    uint16_t diff = target - current;
    return (diff > 240) ? 240 : (uint8_t)diff;
}


void UpdateLeftPanel()
{
  // Left panel
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(LEFTPANEL_TEXT_SIZE);
  tft.setTextDatum(MC_DATUM);
  int yStartP = 38;
  int heighttext = tft.fontHeight();

  tft.fillRect(9, yStartP - 2, 35, (heighttext + 8) * 5, TFT_BLACK);
  drawScanlinesButtons(9, yStartP - 2, 100, 37);
  tft.setCursor(12, yStartP);
  if (WiFi.status() == WL_CONNECTED) 
  {
    tft.drawRect(9, yStartP - 2, 29, heighttext + 4, TFT_GREEN);
    tft.print("WiFi");
  }else
  {
    tft.print("WiFi");
  }
  
  yStartP = yStartP + heighttext + 8;
  tft.setCursor(12, yStartP);
  if (radioIsPlaying()) 
      {
        tft.drawRect(9, yStartP - 2, 29, heighttext + 4, TFT_GREEN);
        tft.print("RAD");
      }else
      {
        tft.print("RAD");
      }

  
  if (GPS_Connected) 
      {
        yStartP = yStartP + heighttext + 8;
        tft.setCursor(12, yStartP);
        if (gpsHasFix()) tft.drawRect(9, yStartP - 2, 29, heighttext + 4, TFT_GREEN);
        tft.print("GPS");
        
      }

  if (pulseSensFound) 
      {
        yStartP = yStartP + heighttext + 8;
        tft.setCursor(12, yStartP);
        if (pulseSensorUpdate()) tft.drawRect(9, yStartP - 2, 35,  heighttext + 4, TFT_GREEN);
        tft.print("PULSE");
      }

  yStartP = yStartP + heighttext + 8;
  tft.setCursor(12, yStartP);
  //tft.fillRect(9, yStartP - 2, 35, 15, TFT_BLACK);
  //drawScanlinesButtons(9, yStartP - 2, 18, 35);
  CurrentWeather* w = weatherGetCurrent();
      String src = (w->source == WEATHER_PRIMARY) ? "[W]" : 
               (w->source == WEATHER_OPENMETEO) ? "[O]" : "[E]";
      //tft.drawString(src, 310, 35);
      if ((src == "[W]") || (src == "[O]"))
      {
        src = "W " + src;
        tft.drawRect(9, yStartP - 2, 35, heighttext + 4, TFT_GREEN);
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

void UpdateRightPanel()
{

  if (currentScreen != 0) return;

      tft.setTextColor(TFT_GREEN);
      tft.setTextSize(TIMERS_TEXT_SIZE);
      tft.setTextDatum(MC_DATUM);

        // Right panel
  
  String Stringbase = "(DDD)SSSSSSSS";
  int widthtext = tft.textWidth(Stringbase);
  int heighttext = tft.fontHeight();

      int XstartRight = TFT_WIDTH_SCREEN - widthtext - 2;
      int YstartRight = 45;

      tft.fillRect(XstartRight, YstartRight - 2, TFT_WIDTH_SCREEN-XstartRight, (heighttext + 8) * 3, TFT_BLACK);
      drawScanlinesButtons(XstartRight, YstartRight-2, (heighttext + 8) * 3 + 2, TFT_WIDTH_SCREEN-XstartRight);
      if (isValidString(T1S)){
        tft.setCursor(XstartRight, YstartRight);
        tft.printf("(%d)%s", minutesUntil(T1h,T1m),T1S);
      }
      if (isValidString(T2S)){
        tft.setCursor(XstartRight, YstartRight + heighttext + 8);
        tft.printf("(%d)%s", minutesUntil(T2h,T2m),T2S);
      }
      if (isValidString(T3S)){
        tft.setCursor(XstartRight, YstartRight + heighttext * 2 + 8 * 2);
        tft.printf("(%d)%s", minutesUntil(T3h,T3m),T3S);
      }

}


// Очистка: оставляем только цифры и ':'
void sanitizeTimeInput(char* input) {
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if ((input[i] >= '0' && input[i] <= '9') || input[i] == ':') {
            input[j++] = input[i];
        }
    }
    input[j] = '\0';
}

void sanitizeGPSInput(char* input) {
    
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if ((input[i] >= '0' && input[i] <= '9') || input[i] == '.' || input[i] == '-' ) {
          if (i == 0)
          {
            if (input[0] == '-' || (input[0] >= '0' && input[0] <= '9'))
              input[j++] = input[0];
          } else
          {
            if (input[i] != '-' )
              input[j++] = input[i];
          }
        }
    }
    input[j] = '\0';
    //input = buf;
    //return buf;
}

// Парсинг времени по флагу
bool parseTime(const char* input) {
    bool parseSuccess = false;
    
    if (input == nullptr || input[0] == '\0') return false;
    char buf[7];
    strncpy(buf, input, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    sanitizeTimeInput(buf);
    
    // --- Режим ЧАСЫ:МИНУТЫ (T1h, T2h, T3h) ---
    if (EditFieldFlag.endsWith("h")) {
        char* colon = strchr(buf, ':');
        
        if (colon != nullptr) {
            // Формат ЧЧ:ММ — парсим и часы, и минуты
            *colon = '\0';
            int h = atoi(buf);
            int m = atoi(colon + 1);
            
            if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
                parsedHours = (uint8_t)h;
                parsedMinutes = (uint8_t)m;
                parseSuccess = true;
            }
        } else {
            // Только часы — минуты НЕ трогаем
            int h = atoi(buf);
            if (h >= 0 && h <= 23) {
                parsedHours = (uint8_t)h;
                parsedMinutes = 99;
                parseSuccess = true;
            }
        }
    }
    // --- Режим МИНУТЫ (T1m, T2m, T3m) ---
    else if (EditFieldFlag.endsWith("m")) {
        int m = atoi(buf);
        if (m >= 0 && m <= 59) {
            parsedMinutes = (uint8_t)m;
            parseSuccess = true;
        }
    }
    
    return parseSuccess;
}

bool isValidString(const String& str)
{
  if (str.length() == 0 ) return false;

  for (size_t i = 0; i < str.length(); i++)
    if (str[i] != ' ') return true;

  return false;
}

String pad2(uint8_t val)
{
  return (val < 10 ? "0" :"") + String(val);
}



void updateWeatherScreen()
{
  // Очищаем всю область контента
  //tft.fillRect(0, SCREEN_HEADER_Y, TFT_WIDTH_SCREEN, SCREEN_CONTENT_H, TFT_BLACK);
  
  // --- РЕЖИМ без BMP — погода по центру экрана ---
  if (!bmpFound) {
    drawWeatherPanelCenter(TFT_WIDTH_SCREEN / 2, SCREEN_HEADER_Y, TFT_WIDTH_SCREEN, SCREEN_CONTENT_H);
    drawUpdateInfo();
    //drawTabButtons();
    //lastScreen = currentScreen;
    return;
  }
  
  // --- РЕЖИМ с BMP — две панели по половине экрана ---
  int panelW = TFT_WIDTH_SCREEN / 2;  // Половина ширины экрана
  int panelH = SCREEN_CONTENT_H - 15;
  int leftX = 0;
  int rightX = panelW;                 // Вторая половина начинается здесь
  
  // Левая панель (BMP) — от 0 до TFT_WIDTH_SCREEN/2
  tft.drawRect(leftX, SCREEN_HEADER_Y, panelW, panelH, TFT_GREEN);
  drawBMPPanel(leftX, SCREEN_HEADER_Y, panelW, panelH);
  
  // Правая панель (Weather) — от TFT_WIDTH_SCREEN/2 до TFT_WIDTH_SCREEN
  tft.drawRect(rightX, SCREEN_HEADER_Y, panelW, panelH, TFT_GREEN);
  drawWeatherPanelSide(rightX, SCREEN_HEADER_Y, panelW, panelH);
  
  // Updated — по центру всего экрана, под рамками
  drawUpdateInfo();
  
  //drawTabButtons();
  //lastScreen = currentScreen;
}


// ============================================================
// ЛЕВАЯ ПАНЕЛЬ — BMP данные
// ============================================================

void drawWeatherPanelCenter(int cx, int y, int w, int h)
{
  if (WiFi.status() != WL_CONNECTED) {
    weatherLoadFromEEPROM();
  }
  
  if (!weatherHasData()) {
    int boxW = w - 10;
    int boxH = 80;
    int boxX = cx - boxW / 2;       // Центрируем бокс
    int boxY = y + h / 2 - boxH / 2;
    
    tft.fillRect(boxX, boxY, boxW, boxH, TFT_BLACK);
    tft.drawRect(boxX, boxY, boxW, boxH, TFT_GREEN);
    
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);
    tft.drawString("No weather data", cx, boxY + boxH / 2 - 10);
    
    if (WiFi.status() != WL_CONNECTED) {
      tft.setTextColor(TFT_RED);
      tft.drawString("Connect WiFi", cx, boxY + boxH / 2 + 10);
    } else {
      tft.setTextColor(TFT_GREEN);
      tft.drawString("Loading...", cx, boxY + boxH / 2 + 10);
      needUpdateScreenWeather = true;
    }
    return;
  }
  
  needUpdateScreenWeather = false;
  CurrentWeather* weatherPtr = weatherGetCurrent();
  
  // --- Температура крупно (size 6) ---
  int tempVal = weatherPtr->temperature.toInt();
  String unit = weatherCelsius ? "C" : "F";
  unit = " " + unit;
  String tempDisplay;
  
  if (tempVal > 0) {
    tempDisplay = "+" + weatherPtr->temperature + unit;
  } else if (tempVal < 0) {
    tempDisplay = "-" + weatherPtr->temperature + unit;
  } else {
    tempDisplay = "0" + unit;
  }
  
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_GREEN);
  int tempFSize = bmpFound ? 4 : 6;
  tft.setTextSize(tempFSize);
  int tw = tft.textWidth("+888 C");
  int th = tft.fontHeight();
  int sx = cx - tw / 2;
      tft.fillRect(sx, y + 10 , tw, th, TFT_BLACK);
      drawScanlinesButtons(sx, y + 10, th, tw);
  tft.drawString(tempDisplay, cx, y + 10);   // cx = TFT_WIDTH_SCREEN / 2
  
  int Widthstr = tft.textWidth(tempDisplay);
  int WidthChar = tft.textWidth("C");
  int circleX = cx + Widthstr / 2 - WidthChar - (WidthChar/2);     // Смещение от центра
  int circleY = y + 15;
  for (int r = 6; r >= 4; r--) {
    tft.drawCircle(circleX, circleY, r, TFT_GREEN);
  }
     


  tft.setTextSize(1);
  tft.setTextDatum(TC_DATUM);

  // --- Ветер — смещение от центра на половину ширины текста ---
  tw = tft.textWidth("Wind:     188 km/h");
  int windX = cx - tw/ 2;
  int windY = ((SCREEN_BOTTOM_Y - y - th - 50)) + (SCREEN_BOTTOM_Y - (SCREEN_BOTTOM_Y - y - th - 50)) / 2 - (16 + 20) / 2;// + 125;

  // --- Иконка погоды ---
  //tft.drawString(tempDisplay, cx, tempY);
  drawWeatherIconCentered(cx, windY - 24, weatherPtr->condition, TFT_GREEN);

  // --- Координаты --- 
  String crdStr = "Coords: " + String(weatherLat.substring(0, 7)) + ", " + String(weatherLon.substring(0, 7));
  tw = tft.textWidth(crdStr);
  int coordX = cx - tw/ 2;
  int coordY = windY + 20;
      tft.fillRect(coordX, windY - 7, tw, 35, TFT_BLACK);
      drawScanlinesButtons(coordX, windY - 7, 35, tw);

  tft.drawString("Wind:      " + String(weatherPtr->wind.c_str()) + " km/h",cx, windY);
  drawWindArrow(windX + 46, windY + 3, weatherPtr->windDir, 20, TFT_WHITE);
  tft.drawString(crdStr,cx, coordY);
 
  // --- Проверка устаревания ---
  if (!weatherHasData() || (WiFi.status() != WL_CONNECTED)) {
    int mins = weatherGetAgeMinutes();
    if (mins > 30) {
      weatherForceUpdate();
      weatherUpdate();
    }
  }
}

void drawWeatherPanelSide(int x, int y, int w, int h)
{
  int cx = x + w / 2;     // Центр правой панели = TFT_WIDTH_SCREEN * 3/4
  //int halfW = w / 2;      // Половина ширины панели
  

  drawWeatherPanelCenter(cx, y, w, h);
 
}

void drawBMPPanel(int x, int y, int w, int h)
{
  if (!bmpMeasure()) return;
  
  int cx = x + w / 2;     // Центр левой панели = TFT_WIDTH_SCREEN / 4
  int halfW = w / 2;
  int pad = 5;
  
  // Иконка
  //drawWeatherIconCentered(cx, y + 20, "indoor", TFT_GREEN);
  
  // Температура крупно
  float temp = bmpGetTemperatureC();
  String tempStr = String(temp, 0);
  if (temp > 0) tempStr = "+" + tempStr; else if (temp < 0) tempStr = "-" + tempStr;
  String unit = weatherCelsius ? "C" : "F";
  tempStr = tempStr + " " + unit;

    tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_GREEN);
  int tempFSize = bmpFound ? 4 : 6;
  tft.setTextSize(tempFSize);
  int tw = tft.textWidth("+888 C");
  int th = tft.fontHeight();
  int sx = cx - tw / 2;
      tft.fillRect(sx, y+10, tw, th, TFT_BLACK);
      drawScanlinesButtons(sx, y+10, th, tw);
  tft.drawString(tempStr, cx, y + 10); 

  int Widthstr = tft.textWidth(tempStr);
  int WidthChar = tft.textWidth("C");
  int circleX = cx + (Widthstr / 2) - WidthChar - (WidthChar/2);     // Смещение от центра
  int circleY = y + 15;
  for (int r = 6; r >= 4; r--) {
    tft.drawCircle(circleX, circleY, r, TFT_GREEN);
  }
  // "INDOOR"
  tft.setTextSize(1);
  tft.setTextColor(tft.color565(0, 150, 0));
  tft.drawString("INDOOR", cx, y + 10 + th);
  
  // Разделитель
  int lineY = y + th + 20;
  tft.drawLine(x + pad, lineY, x + w - pad, lineY, tft.color565(0, 80, 0));
  
  // Данные мелким шрифтом
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  
  int rowY = lineY + 10;
  int rowH = 16;
  int labelX = x + pad + 2;         // От левого края панели
  int valueX = x + halfW + 5;      // Правая половина панели

  if (activeSensor == SENSOR_BME280) {
        tft.setCursor(labelX, rowY);
        tft.print("Humidity:");
        tft.setCursor(valueX, rowY);
        tft.printf("%.1f %%", bmpGetHumidity());
        rowY += rowH;
    }

  tft.setCursor(labelX, rowY);
  tft.print("Pressure:");
  tft.setCursor(valueX, rowY);
  tft.printf("%.1f mmHg", bmpGetPressureMmHg());
  rowY += rowH;
  
  tft.setCursor(labelX, rowY);
  tft.print("Pressure:");
  tft.setCursor(valueX, rowY);
  tft.printf("%.2f kPa", bmpGetPressureKpa());
  rowY += rowH;
  
  tft.setCursor(labelX, rowY);
  tft.print("Altitude:");
  tft.setCursor(valueX, rowY);
  tft.printf("%+.1f m", bmpGetRelativeAltitude());
  rowY += rowH;
  
  tft.setCursor(labelX, rowY);
  tft.print("Sea level:");
  tft.setCursor(valueX, rowY);
  tft.printf("%.1f m", bmpGetSeaLevelAltitude());


}


// ============================================================
// Время обновления на экране weather — под рамками, над TAB_H
// ============================================================
void drawUpdateInfo()
{
  CurrentWeather* w = weatherGetCurrent();
  if (!weatherHasData()) return;
  
  int mins = weatherGetAgeMinutes();
  if (mins < 0) return;
  
  bool isOld = (mins > 60);

  String stringw = "";

  if (isOld) 
  {
    if (weatherLastUpdate())
    {
      time_t dateUpW = weatherLastUpdate();
      time_t local = myTZ.toLocal(dateUpW, &tcr);
      stringw = pad2(day(local)) + "." + pad2(month(local)) + "." + String(year(local)) + " " + pad2(hour(local)) + ":" + pad2(minute(local));
    }
    else
    {
      mins = 60;
      stringw = String(mins); 
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    if (!isOld)
    {
      stringw = stringw + String(mins) + " min ago";
    }
  } else {
    if (!isOld)
    {
      stringw = stringw + String(mins) + " min ago" + " (WiFi OFF)";
    }
    else
    {
      stringw = stringw + " (WiFi OFF)";
    }
  }
  tft.setTextSize(1);
  int tw = tft.textWidth(stringw);
  int tuw = tft.textWidth("Updated: ");
  int th = tft.fontHeight();
  tft.fillRect(TFT_WIDTH_SCREEN / 2 - tw / 2 - tuw, SCREEN_BOTTOM_Y - th, tuw + tw, th, TFT_BLACK);
  drawScanlinesButtons(TFT_WIDTH_SCREEN / 2 - tw / 2 - tuw, SCREEN_BOTTOM_Y - th, th, tuw + tw); 
  tft.setTextColor(tft.color565(0, 100, 0));
  tft.setTextDatum(BR_DATUM);
  tft.drawString("Updated: ", TFT_WIDTH_SCREEN / 2 - tw / 2, SCREEN_BOTTOM_Y);
  tft.setTextColor(isOld ? TFT_ORANGE : TFT_GREEN);
  tft.setTextDatum(BC_DATUM);
  tft.drawString(stringw, TFT_WIDTH_SCREEN / 2, SCREEN_BOTTOM_Y);

  // Индикатор источника
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(tft.color565(0, 100, 0));
  // Закрасить старое значение
  String src = (w->source == WEATHER_PRIMARY) ? "[W]" : 
               (w->source == WEATHER_OPENMETEO) ? "[O]" : "[E]";
  int tws = tft.textWidth(src);             
  tft.fillRect(TFT_WIDTH_SCREEN - tws - 5, 10, tws + 5, th, TFT_BLACK); 
  drawScanlinesButtons(TFT_WIDTH_SCREEN - tws - 5, 10, th+2, tws + 5);
  tft.drawString(src, TFT_WIDTH_SCREEN - tws - 5, 10);

  
}

bool pulseSensorUpdate()
{
  
      tft.setTextSize(1);
      tft.setTextDatum(TR_DATUM);
      tft.setTextColor(TFT_GREEN);
     String src = " BPM:300 SPO2:000";
     int tws = tft.textWidth(src);  
            
   if (!pulseSensorConnected && pulseSensFound) {
            if (DEBUGFLAG)  Serial.println("[UI PULSE] Pulse not connected");
        } else {
            PulseData pd = pulseGetData();
              int raw = pd.spo2;
              int bpm = pd.bpm;

            //if (DEBUGFLAG)  Serial.printf("[UI PULSE] BPM:%d SpO2:%d%%\n", pd.bpm, pd.spo2);

            if (pd.status == PULSE_FINGER_NOT_DETECTED) {
                //if (DEBUGFLAG)  Serial.println("[UI PULSE] Finger out");
                if (DEBUGFLAG)  {
                tft.fillRect(5, SCREEN_BOTTOM_Y - 82, tws, 10, TFT_BLUE);
                tft.drawString("Body Not detected", tws + 5, SCREEN_BOTTOM_Y - 80);
                }
                return false;
            } else if (pd.status = PULSE_CALIBRATING) {
                //bpm = 0;
                if (DEBUGFLAG)  Serial.print("[UI PULSE] Calibrate\n");
            } else if (pd.status == PULSE_OK) {
                if (DEBUGFLAG)  Serial.printf("[UI PULSE] BPM:%d SpO2:%d%%\n", pd.bpm, pd.spo2);
            }
    

if (DEBUGFLAG && pulseSensFound) 
    {
      // Закрасить старое значение           
      tft.fillRect(5, SCREEN_BOTTOM_Y - 82, tws, 10, TFT_BLUE);
      if (pd.status = PULSE_CALIBRATING)
      {
        tft.drawString("Calibrating..", tws + 5, SCREEN_BOTTOM_Y - 80);
      } else if (pd.status == PULSE_OK) {
        src = "BPM:" + String(bpm) + " SpO2:" + String(raw);
        tft.drawString(src, tws + 5, SCREEN_BOTTOM_Y - 80);
      }
      
      //if (DEBUGFLAG) { Serial.printf("[UI PULSE] Status: %s\n", pulseStatusToString(pd.status)); }
    }



  if (bpm > 0)
  {  
    currentHP = bpm;
    currentAP = raw;// / 20;

    return true;

  } else {

    currentAP = raw;// / 20;
    return false; 
  }
        }
}

void clickBuzzer()
{
  tone(BUZZER_PIN, 220 , 10);
  //delay(25);
  noTone(BUZZER_PIN);
}

void clickBuzzerKey()
{

 int dtmfk[] = {1187, 697, 4770, 941, 770, 1997, 1141, 741};
 //for (int i = 0; i < (sizeof(dtmf) / sizeof(dtmf[0])); i ++)

  //int dtflag = random(0, 1);
  int rdeldtk = random(10, 60);
  int idxdtk = random(0, (sizeof(dtmfk) / sizeof(dtmfk[0])));
  tone(BUZZER_PIN, dtmfk[idxdtk] , rdeldtk);
  //tone(BUZZER_PIN, 220 , 10);
  //delay(25);
  noTone(BUZZER_PIN);
}

void clackBuzzer()
{
  
 int dtmf[] = {1187, 697, 4770, 941, 770, 1997, 1141, 741};
 //for (int i = 0; i < (sizeof(dtmf) / sizeof(dtmf[0])); i ++)

int dtflag = random(0, 1);
int rdeldt = random(10, 60);
int idxdt = random(0, (sizeof(dtmf) / sizeof(dtmf[0])));

tone(BUZZER_PIN, 8220 , 10);
delay(50);
 for (int i = 0; i < 6; i++)
  {
    idxdt = random(0, (sizeof(dtmf) / sizeof(dtmf[0])));
    rdeldt = random(10, 60);
    dtflag = random(0, 3);
    if (dtflag)
    tone(BUZZER_PIN, dtmf[idxdt] , rdeldt);
    delay(rdeldt + random(1, 10));
    //dtflag = random(0, 1);
    //if (dtflag)
     
  }
  tone(BUZZER_PIN, 120 , 10);
  //   delay(20);
  noTone(BUZZER_PIN);
}




// Вспомогательная функция — плавное сканирование частоты

void sweepTone(int startFreq, int endFreq, int durationMs, int stepMs = 10) {
  int steps = durationMs / stepMs;
  float delta = (float)(endFreq - startFreq) / steps;
  ledcWrite(BUZZER_PIN, 128);
  for (int i = 0; i <= steps; i++) {
    int f = startFreq + (int)(delta * i);
    tone(BUZZER_PIN, f, stepMs);
    delay(stepMs);
  }
  noTone(BUZZER_PIN);
}

void startBuzzer()
{
  sweepTone(800, 15000, 400, 8);
  //delay(100);
}


// === ЗВУК ЗАГРУЗКИ PIP-BOY ===
void pipboyBootSound() {
  // 1. POWER ON — щелчок реле (короткий низкий «тик»)
  tone(BUZZER_PIN, 150, 20);
  delay(30);
  noTone(BUZZER_PIN);
  delay(80);

  // 2. CRT WARM-UP — нарастающий писк вакуумной трубки
  // Имитирует разогрев катода: 800 Гц → 15 кГц, 600 мс
  sweepTone(800, 15000, 600, 8);
  delay(100);

  // 3. TUBE HUM — низкочастотный гул ЭЛТ
  // Пьезо плохо играет <200 Гц, поэтому имитируем «треск» гула серией импульсов
  for (int i = 0; i < 15; i++) {
    tone(BUZZER_PIN, 80 + random(0, 40), 12);
    delay(20);
  }
  noTone(BUZZER_PIN);
  delay(150);

  // 4. DATA READ — быстрые электронные щелчки чтения памяти
  // Как у старого дисковода, но короче
  for (int burst = 0; burst < 4; burst++) {
    for (int i = 0; i < 6; i++) {
      int f = random(2000, 6000);
      tone(BUZZER_PIN, f, random(15, 40));
      delay(random(30, 70));
    }
    delay(random(80, 150)); // пауза между пакетами данных
  }

  // 5. SCREEN BLOOM — «расплывание» изображения
  // Плавное падение высокой частоты + треск
  sweepTone(12000, 3000, 400, 12);
  for (int i = 0; i < 8; i++) {
    tone(BUZZER_PIN, random(1000, 4000), 15);
    delay(25);
  }
  delay(200);

  // 6. BOOT COMPLETE — два коротких ретро-бипа (как в Fallout UI)
  tone(BUZZER_PIN, 1800, 80);
  delay(120);
  tone(BUZZER_PIN, 2200, 120); // чуть выше и длиннее — «готово»
  delay(150);
  noTone(BUZZER_PIN);
}


// Неблокирующий радиационный треск (счётчик Гейгера)
// level: 0=выкл, 1=фоновая радиация, 10=критическая масса
void geigerTick(int level) {
  if (radioIsPlaying())
    return;
  static unsigned long lastTick = 0;
  static unsigned long toneEnd = 0;
  static unsigned long pauseMs = 0;
  static bool sounding = false;

  // Полное отключение
  if (level <= 0) {
    if (sounding) {
      noTone(BUZZER_PIN);
      sounding = false;
    }
    lastTick = millis();
    pauseMs = 0;
    return;
  }

  if (level > 15) level = 15;

  // Фаза воспроизведения щелчка
  if (sounding) {
    if (millis() >= toneEnd) {

      //digitalWrite(BUZZER_PIN, LOW);
      noTone(BUZZER_PIN);
      sounding = false;
      // Случайная пауза до следующего щелчка (уровень сокращает интервал)
      int minPause = map(level, 1, 14, 1200, 50);
      int maxPause = map(level, 1, 14, 5000, 260);
      pauseMs = random(minPause, maxPause + 1);
      lastTick = millis();
    }
    return;
  }

  // Инициализация при первом запуске
  if (pauseMs == 0) {
    pauseMs = random(800, 2000);
  }

  // Проверяем, не подошло ли время щёлкнуть
  if (millis() - lastTick >= pauseMs) {
    int freq = random(50, 85);      // случайная высота щелчка
    int duration = random(5, 8);       // случайная длительность
    //ledcWrite(BUZZER_PIN, 128);
    tone(BUZZER_PIN, freq);
    noTone(BUZZER_PIN);
    //pinmode
    //digitalWrite(BUZZER_PIN, HIGH);
    toneEnd = millis() + duration;
    sounding = true;
  }
}

bool NeedSyncDataTime()
{
  DateTime rtcNow = rtc.now();
  if (eepromUpdateDataTime = 0)
  {
    if (SyncLoadFromEEPROM())
    {
        if (year(eepromUpdateDataTime) != rtcNow.year() ||  month(eepromUpdateDataTime) != rtcNow.month() || day(eepromUpdateDataTime) !=  rtcNow.day())
          return true;
        else
          return false;
    } else
    {
    if (year(eepromUpdateDataTime) != rtcNow.year() || month(eepromUpdateDataTime) != rtcNow.month() || day(eepromUpdateDataTime) !=  rtcNow.day())
          return true;
        else
          return false;
    }
  }
  return true;
}

// ======================= EEPROM =======================


bool SyncLoadFromEEPROM() {
  LastSyncDateTime data;
  
  // Очистка перед чтением
  memset(&data, 0, sizeof(LastSyncDateTime));
  
  if (!eepromReadSlot(5, (uint8_t*)&data)) {
    if (DEBUGFLAG) Serial.println("[SYNC] read SLOT 5 --- failed");
    return false;
  }
  
  // Загрузка данных

  //eepromDataTime = data.timestamp;
  // Восстановление времени
  time_t rtcNow = now();
  eepromUpdateDataTime = data.lastSyncDT;

  if (eepromUpdateDataTime > rtcNow) {
   rtc.adjust(DateTime(year(eepromUpdateDataTime), month(eepromUpdateDataTime), day(eepromUpdateDataTime), 
                     hour(eepromUpdateDataTime), minute(eepromUpdateDataTime), second(eepromUpdateDataTime)));
    Serial.print("[SYNC] Date in RTC older then in Time Sync backup. Adjust RTC.\n");
    DateTime rtcNow = rtc.now();
    setTime(rtcNow.unixtime());
  }

  #if DEBUGFLAG
  Serial.print("[SYNC] EEPROM SLOT 5 loaded OK:");
    char stringwu[32];
    snprintf(stringwu, sizeof(stringwu), "%02d.%02d.%04d %02d:%02d:%02d", day(eepromUpdateDataTime), month(eepromUpdateDataTime), year(eepromUpdateDataTime), hour(eepromUpdateDataTime), minute(eepromUpdateDataTime), second(eepromUpdateDataTime));
    Serial.printf(" %s\n", stringwu); 
  #endif
  return true;
}


void SyncSaveToEEPROM() {
  LastSyncDateTime data;
  
  // Очистка всей структуры перед использованием!
  memset(&data, 0, sizeof(LastSyncDateTime));
  
  data.lastSyncDT = (uint32_t)now();
  eepromUpdateDataTime = now();

  if (eepromWriteSlot(5, (uint8_t*)&data)) {
   if (DEBUGFLAG)  Serial.println("[SYNC] Date time saved to SLOT 5 EEPROM");
  }
}
