#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

//#define CYD2_4        // Uncomment if you use 2.4 CYD and in USER_SETUP.h
#define CYD3_5          // Uncomment if you use 3.5 CYD
#define DEBUGFLAG false  // TRUE if you need Serial monitor debug & GPS blue window debug on screen & pulse debug info

#define PERSON_NAME "SAM"
/*
// ======================= TFT PINS SETUP (2.4 CYD Type C) =======================
// ===================  edit UserSetup.h in TFT_eSPI library =====================
//====== CYD 3.5 -----------
#define ST7796_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480
#define TFT_BL    27
#define TFT_INVERSION_OFF
#define TFT_RGB_ORDER TFT_BGR   // Colour order Blue-Green-Red
#define TOUCH_CS 33
//====== CYD 2.4----------
#define ILI9341_2_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_BL    27
#define TFT_INVERSION_ON
#define TFT_RGB_ORDER TFT_BGR   // Colour order Blue-Green-Red
#define TOUCH_CS 33

// ======================= TFT PINS SETUP =======================
// edit UserSetup.h in TFT_eSPI library
#define ESP32_DMA
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   -1
#define TFT_MOSI  13
#define TFT_MISO  12
#define TFT_SCLK  14

#define TFT_BACKLIGHT_ON HIGH
*/

#ifdef CYD2_4
  #define TFT_WIDTH_SCREEN  320
  #define TFT_HEIGHT_SCREEN 240
#endif

#ifdef CYD3_5
  #define TFT_WIDTH_SCREEN  480
  #define TFT_HEIGHT_SCREEN 320
#endif

#define SPI_FREQUENCY  80000000
#define SPI_READ_FREQUENCY  80000000
#define SPI_TOUCH_FREQUENCY  2500000
#define USE_HSPI_PORT

// =================  RGB LED PINS (активный LOW)
#ifdef CYD3_5
  #define LED_R     4
  #define LED_G     16
  #define LED_B     17
#endif

#ifdef CYD2_4
  #define LED_R     4
  #define LED_G     17
  #define LED_B     16
#endif

// ========================== SD CARD PINS ==========================
#define FS_NO_GLOBALS
#define SD_CS 5
#define SD_SCLK 18
#define SD_MISO 19
#define SD_MOSI 23
// ======================= RTC НАСТРОЙКИ =======================
#define RTC_ADDRESS 0x68
#define RTC_SDA 21
#define RTC_SCL 22
#define GMT_SET +3 // ( or -3 etc)

// ======================= EEPROM AT24C32 =======================
#define EEPROM_ADDRESS 0x50 // or 0x57
#define EEPROM_PAGE_SIZE 32
#define EEPROM_SIZE 4096


// ======================= BMP280 or BME280 барометр =======================
#define BMP280_ADDRESS 0x76
#define OFFSET_TEMP
#define BASE_ALT

// ======================= Pulse Sensor MAX30102 pin i2c ======================= 

// --- Настройки как в примере, но под MAX30102 ---
#define PULSE_I2C_SDA             RTC_SDA
#define PULSE_I2C_SCL             RTC_SCL
#define MAX30102_I2C_ADDR         0x57
#define PULSE_LED_BRIGHTNESS      0x1F //0x7F  // 127 как в примере | Options: 0=Off to 255=50mA
#define PULSE_SAMPLE_AVERAGE      8    // //Options: 1, 2, 4, 8, 16, 32 | 1 = без усреднения! (как в примере)
#define PULSE_LED_MODE            2     // 2 = Red+IR (для MAX30102) | Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
#define PULSE_SAMPLE_RATE         200   // 100 Гц  | Options: 50, 100, 200, 400, 800, 1000, 1600, 3200 | 100-200 optimal
#define PULSE_PULSE_WIDTH         411    // 69 мкс (как в примере) | Options: 69, 118, 215, 411
#define PULSE_ADC_RANGE           4096 //4096  // как в примере | Options: 2048, 4096, 8192, 16384
#define BUFFER_LENGTH             100     // для графика в углу. min = 60 | for graph in corner on main screen BUFFER_LENGTH >= PULSE_SAMPLE_RATE
#define PULSE_FINGER_IR_THRESHOLD 50000 //3000
#define CALIBRATION_TIMEOUT_MS    8000
  
#define PULSE_FINGER_RED_MIN      50000
#define TEMP_READ_INTERVAL_MS     2000


// ======================= GPS GY-NE06MV2 ======================= 


// ============================================
// ВЫБЕРИТЕ ОДИН ИЗ ТРЁХ РЕЖИМОВ подключения GPS:
// ============================================

// Режим 1: GPS через I2C с ESP32-C3 (умный мост + LED)
 //#define GPS_USE_I2C_C3

// Режим 2: GPS напрямую через UART (стандартный NEO-6M)
// #define GPS_USE_UART

// Режим 3: GPS через I2C-UART мост SC16IS750
//#define GPS_USE_I2C_SC16IS750

#ifdef CYD3_5
  #define GPS_USE_UART // or
  // #define GPS_USE_I2C_SC16IS750 // or
  // #define GPS_USE_I2C_C3
#endif

#ifdef CYD2_4
  //#define GPS_USE_I2C_C3 // or 
  #define GPS_USE_UART
  // #define GPS_USE_I2C_SC16IS750
#endif

// ============================================
// НАСТРОЙКИ I2C (для режима C3)
// ============================================
#ifdef GPS_USE_I2C_C3
  #define C3_I2C_ADDR         0x10
  #define GPS_I2C_PACKET_SIZE 34   // magic(1) + data(32) + checksum(1)
  #define GPS_PACKET_MAGIC    0xC3
#endif

// ============================================
// НАСТРОЙКИ I2C-SC16IS750 (для режима моста)
// ============================================
#ifdef GPS_USE_I2C_SC16IS750
  #define GPS_SC16IS750_ADDR   0x48      // 7-bit I2C адрес (A0=A1=GND → 0x48)
  #define GPS_SC16IS750_XTAL   1843200UL // кварц на модуле в Гц (1.8432 МГц стандарт) 1843200UL
  // Если на вашем модуле кварц 12 МГц — укажите 12000000UL
#endif

// ============================================
// НАСТРОЙКИ UART (для прямого режима и моста)
// ============================================
#if defined(GPS_USE_UART) || defined(GPS_USE_I2C_SC16IS750)
  #define GPS_UART_BAUD       9600
#endif

#ifdef GPS_USE_UART                   // проверить, что бы не было пересечения с #define BUZZER_PIN
  #define GPS_UART_NUM        2       // HardwareSerial(2)
  #define GPS_UART_RX_PIN     25      // <-- Укажи свои пины! FPC 0.5 6p connector 3 pin
  #define GPS_UART_TX_PIN     32      // <-- Укажи свои пины! FPC 0.5 6p connector 4 pin
#endif

#define CHANGE_PIP_COLOR 1
#define MAP_START_X 5 //90 // 5
#define MAP_START_Y 35 //45 //35
#define MAP_END_X (TFT_WIDTH_SCREEN - 55) //460
#define MAP_END_Y TAB_Y - 20 // 245
#define MAP_ZOOM_OUT 12
#define MAP_ZOOM_IN 16


// ======================= TOF10120 Time-of-Flight Laser Distance Sensor ======================= 
// ============================== I2C interface  =========================== 

// I2C адрес (7-bit). В даташите указан 0xA4 (8-bit), 
// но библиотека Wire использует 7-bit адресацию -> 0x52 (82)
#define TOF10120_I2C_ADDR         0x52
// Минимальный интервал между опросами, мс (по даташиту ≥30 мс)
#define TOF10120_READ_INTERVAL    150
// I2C таймаут на одну транзакцию, мс
#define TOF10120_I2C_TIMEOUT      100
// Диапазон валидации, мм
#define TOF10120_MIN_DISTANCE     20
#define TOF10120_MAX_DISTANCE     2000
// Значение при ошибке чтения
#define TOF10120_ERROR_VALUE      -1
// Размер окна скользящего среднего (фильтр)
#define TOF10120_FILTER_WINDOW    5

// ======================= WiFi / NTP =======================
#define NTP_SERVER "pool.ntp.org"
#define NTP_SYNC_INTERVAL 36000      // секунд
#define WIFI_TIMEOUT 10000            // мс

// ======================= WEATHER =======================
#define WEATHER_UPDATE_INTERVAL 900000  // 15 минут в мс
#define WEATHER_TIMEOUT_WTTR 15000       // 15 сек
#define WEATHER_TIMEOUT_OPENMETEO 10000 // 10 сек

#define WEATHER_PRIMARY   0
#define WEATHER_OPENMETEO 1
#define WEATHER_EEPROM    2

// ======================= RADIO =======================
// Настройки пинов
#define RADIO_DAC_PIN 26        // GPIO26 - DAC_CHANNEL_2
#define RADIO_VOLUME_DEFAULT 50 // 0-100 (громкость усилителя 8002D)

#ifdef CYD2_4
  #define BUZZER_PIN LED_G
#endif

#ifdef CYD3_5
  #define BUZZER_PIN LED_G  
#endif

// =============== Weather ================
//Координаты GPS по умолчанию для погоды (Москва)
#define DEFAULT_LAT "55.7558"
#define DEFAULT_LON "37.6173"

// ======================= UI ========================
#define TAB_COUNT 6

#ifdef CYD3_5
  #define TAB_H 40   // нижние кнопки
  #define LVL_TEXT_SIZE 3
#endif

#ifdef CYD2_4
  #define TAB_H 35   // нижние кнопки
  #define LVL_TEXT_SIZE 2 
#endif

#define TAB_Y (TFT_HEIGHT_SCREEN - TAB_H - 2)
#define TAB_W (TFT_WIDTH_SCREEN/TAB_COUNT)

#define TAB_TEXT_SIZE 1

#define TIMERS_TEXT_SIZE 1
#define LEFTPANEL_TEXT_SIZE 1
#define KEYBOARD_X 5
#define KEYBOARD_Y 5
#define KEY_GAP 2
#define KEYBOARD_W (TFT_WIDTH_SCREEN - KEYBOARD_X * 2)
#define KEYBOARD_H (TFT_HEIGHT_SCREEN - KEYBOARD_Y * 2)
#define KEY_W ((KEYBOARD_W - KEY_GAP * 12) / 10 )//28
#define KEY_H ((KEYBOARD_H - KEYBOARD_Y - 28 - 22 - KEY_GAP * 14)/6 )//24 KEYBOARD_H - KEY_H - KEY_GAP - 4

#define VBOYSTARTX ((TFT_WIDTH_SCREEN - 180) / 2)
#define VBOYSTARTY (28 + ((TFT_HEIGHT_SCREEN - 180 - TAB_H - 25) / 2))
// ================== UI SETUPS SCREEN dimensions =============
#define  SCREEN_HEADER_Y  40      // Нижняя граница заголовка "WEATHER"
#define  SCREEN_BOTTOM_Y  (TFT_HEIGHT_SCREEN - TAB_H - 5) // Верхняя граница TAB_H
#define  SCREEN_CONTENT_H  (SCREEN_BOTTOM_Y - SCREEN_HEADER_Y) // Доступная высота 
#define  SCREEN_X 85
#define  SCREEN_Y 10
#define  SCREEN_H (SCREEN_BOTTOM_Y - SCREEN_Y)
#define  SCREEN_W (TFT_WIDTH_SCREEN - LIST_X - 5)
#define  SCREEN_CENTER (SCREEN_X + (TFT_WIDTH_SCREEN - SCREEN_X - 5)/2)

#define  INPUT_FIELD_H 20   // высота поля ввода


#ifdef CYD3_5
  #define  BUTTON_H 30
#endif

#ifdef CYD2_4
  #define  BUTTON_H 25
#endif

// ================== UI RADIO BUTTONS =================
#define RADIO_B_X ((TFT_WIDTH_SCREEN / 6) - 2)
#define RADIO_B_Y (TAB_Y - TAB_H - 5)
#define RADIO_B_H TAB_H

// ================== WiFi list dimensions =============
#define LIST_X 85
#define LIST_Y 40
#define LIST_W (TFT_WIDTH_SCREEN - LIST_X  - 35 - 5 )
#define LIST_H (TFT_HEIGHT_SCREEN - LIST_Y- TAB_H - 10)
#define LIST_ITEM_H 20
#define MAX_NETWORKS 15


#endif
