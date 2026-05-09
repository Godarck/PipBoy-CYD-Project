#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define DEBUGFLAG true

#define PERSON_NAME "SAM"
// ======================= TFT PINS SETUP (2.4 CYD Type C) =======================
// edit UserSetup.h in TFT_eSPI library
#define ST7796_DRIVER
#define ESP32_DMA
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   -1
#define TFT_MOSI  13
#define TFT_MISO  12
#define TFT_SCLK  14
#define TFT_BL    27
#define TFT_BACKLIGHT_ON HIGH
#define TOUCH_CS 33

#define TFT_WIDTH_SCREEN  480
#define TFT_HEIGHT_SCREEN 320

#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red

#define TFT_INVERSION_OFF

//#define TFT_WIDTH  320
//#define TFT_HEIGHT 480
#define SPI_FREQUENCY  80000000
#define SPI_READ_FREQUENCY  80000000
#define SPI_TOUCH_FREQUENCY  2500000
#define USE_HSPI_PORT

// RGB LED PINS (активный LOW)
#define LED_R     4
#define LED_G     16
#define LED_B     17
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


// ======================= BMP280 барометр =======================
#define BMP280_ADDRESS 0x76

// ======================= Pulse Sensor 3 pin Analog ======================= 
#define PULSE_SENSOR_PIN    255 
//#define PULSE_SENSOR_VCC 8

// ======================= GPS GY-NE06MV2 ======================= 
#define GPS_ONTX_PIN 32
#define GPS_ONRX_PIN 35

// ======================= WiFi / NTP =======================
#define NTP_SERVER "pool.ntp.org"
#define NTP_SYNC_INTERVAL 3600      // секунд
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

#define BUZZER_PIN 32
// =============== Weather ================
//Координаты GPS по умолчанию для погоды (Москва)
#define DEFAULT_LAT "55.7558"
#define DEFAULT_LON "37.6173"

// ======================= UI ========================
#define TAB_H 40
#define TAB_Y (TFT_HEIGHT_SCREEN - TAB_H - 2)
#define TAB_W (TFT_WIDTH_SCREEN/5)

#define TAB_TEXT_SIZE 2
#define LVL_TEXT_SIZE 3
#define TIMERS_TEXT_SIZE 1
#define LEFTPANEL_TEXT_SIZE 1
#define KEYBOARD_X 5
#define KEYBOARD_Y 5
#define KEY_GAP 2
#define KEYBOARD_W (TFT_WIDTH_SCREEN - KEYBOARD_X * 2)
#define KEYBOARD_H (TFT_HEIGHT_SCREEN - KEYBOARD_Y * 2)
#define KEY_W ((KEYBOARD_W - KEY_GAP * 12) / 10 )//28
#define KEY_H ((KEYBOARD_H - KEYBOARD_Y - 28 - 22 - KEY_GAP * 14)/6 )//24 KEYBOARD_H - KEY_H - KEY_GAP - 4

// ================== UI SETUPS SCREEN dimensions =============
#define  SCREEN_HEADER_Y  40      // Нижняя граница заголовка "WEATHER"
#define  SCREEN_BOTTOM_Y  (TFT_HEIGHT_SCREEN - TAB_H - 5) // Верхняя граница TAB_H
#define  SCREEN_CONTENT_H  (SCREEN_BOTTOM_Y - SCREEN_HEADER_Y) // Доступная высота 
#define  SCREEN_X 85
#define  SCREEN_Y 10
#define  SCREEN_H (SCREEN_BOTTOM_Y - SCREEN_Y)
#define  SCREEN_W (TFT_WIDTH_SCREEN - LIST_X - 5)
#define  SCREEN_CENTER (SCREEN_X + (TFT_WIDTH_SCREEN - SCREEN_X - 5)/2)
#define  BUTTON_H 30
#define  INPUT_FIELD_H 20

// ================== UI RADIO BUTTONS =================
#define RADIO_B_X ((TFT_WIDTH_SCREEN / 6) - 2)
#define RADIO_B_Y (TAB_Y - TAB_H - 5)
#define RADIO_B_H TAB_H

// ================== WiFi list dimensions =============
#define LIST_X 85
#define LIST_Y 40
//#define tab_b_W  70
#define LIST_W (TFT_WIDTH_SCREEN - LIST_X  - 35 - 5 )
#define LIST_H (TFT_HEIGHT_SCREEN - LIST_Y- TAB_H - 10)
#define LIST_ITEM_H 20
#define MAX_NETWORKS 15

// ======================= DEBUG =======================
//void Debug(String label, uint8_t val)

#endif
