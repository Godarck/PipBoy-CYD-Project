#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ======================= TFT ПИНЫ (2.4 CYD Type C) =======================
// edit UserSetup.h in TFT_eSPI library
#define ILI9341_2_DRIVER
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

#define TFT_WIDTH_SCREEN  320
#define TFT_HEIGHT_SCREEN 240

#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red

#define TFT_INVERSION_ON

#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define SPI_FREQUENCY  80000000
#define SPI_READ_FREQUENCY  80000000
#define SPI_TOUCH_FREQUENCY  2500000
#define USE_HSPI_PORT

// RGB LED (активный LOW)
#define LED_R     4
#define LED_G     17
#define LED_B     16
// ========================== SD CARD ==========================
#define FS_NO_GLOBALS
#define SD_CS 5
#define SD_SCLK 18
#define SD_MISO 19
#define SD_MOSI 23
// ======================= RTC НАСТРОЙКИ =======================
#define RTC_ADDRESS 0x68
#define RTC_SDA 21
#define RTC_SCL 22

// ======================= EEPROM AT24C32 =======================
#define EEPROM_ADDRESS 0x50 // or 0x57
#define EEPROM_PAGE_SIZE 32
#define EEPROM_SIZE 4096

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
#define RADIO_VOLUME_DEFAULT 10 // 0-21 (громкость усилителя 8002D)


// Координаты по умолчанию для погоды (Москва)
//#define DEFAULT_LAT "55.7558"
//#define DEFAULT_LON "37.6173"

#define DEFAULT_LAT "55.594996"
#define DEFAULT_LON "38.121188"


// ======================= UI =======================
#define TAB_Y 205
#define TAB_H 32
#define TAB_W 64

#define KEYBOARD_X 5
#define KEYBOARD_Y 5
#define KEYBOARD_W 310
#define KEYBOARD_H 235
#define KEY_W 28
#define KEY_H 24
#define KEY_GAP 2

// WiFi list dimensions
#define LIST_X 85
#define LIST_Y 40
#define LIST_W 190
#define LIST_H 160
#define LIST_ITEM_H 20
#define MAX_NETWORKS 15

// ======================= DEBUG =======================
#define DEBUG_ENABLED true
void debugPrint(const char* label, int value);
void debugPrint(const char* label, const char* value);

#endif
