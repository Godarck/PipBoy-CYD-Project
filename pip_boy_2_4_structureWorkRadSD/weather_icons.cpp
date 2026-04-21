#include "weather_icons.h"
#include "weather_module.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "rtc_module.h"  // или ваша библиотека для RTC

// Внешняя ссылка на tft (из вашего основного файла)
extern TFT_eSPI tft;
extern CurrentWeather currentWeather;

// === ОПРЕДЕЛЕНИЕ ВРЕМЕНИ СУТОК ===
bool isDayTime() {
	time_t t = now();
  int h = hour(t);
  // День: с 7:00 до 20:00
  return (h >= 7 && h < 20);
}

// === ОСНОВНАЯ ФУНКЦИЯ ОТРИСОВКИ ИКОНКИ ===
void drawWeatherIcon(int x, int y, String condition, uint16_t color) {
  condition.toLowerCase();
  bool day = isDayTime();
  
  const unsigned char* icon = nullptr;
  
  // 1. ЯСНО / CLEAR / SUNNY
  if (condition.indexOf("clear") >= 0 || condition == "sunny") {
    icon = day ? met_bitmap_black_50x50_clearsky_day : met_bitmap_black_50x50_clearsky_night;
  }
  
  // 2. МАЛООБЛАЧНО / FAIR / PARTLY CLOUDY / MAINLY CLEAR
  else if (condition.indexOf("fair") >= 0 || 
           condition.indexOf("partly cloudy") >= 0 || 
           condition.indexOf("mainly clear") >= 0) {
    if (day) {
      icon = met_bitmap_black_50x50_fair_day;
    } else {
      icon = met_bitmap_black_50x50_fair_night;
    }
  }
  
  // 3. ОБЛАЧНО / CLOUDY / OVERCAST
  else if (condition.indexOf("cloud") >= 0 || condition.indexOf("overcast") >= 0) {
    icon = met_bitmap_black_50x50_cloudy;
  }
  
  // 4. ЛИВНИ / SHOWERS (с дождем)
  else if (condition.indexOf("showers") >= 0 && condition.indexOf("rain") >= 0) {
    if (day) {
      icon = met_bitmap_black_50x50_rainshowers_day;
    } else {
      icon = met_bitmap_black_50x50_rainshowers_night;
    }
  }
  
  // 5. СНЕГОПАД / SNOW SHOWERS
  else if (condition.indexOf("showers") >= 0 && condition.indexOf("snow") >= 0) {
    if (day) {
      icon = met_bitmap_black_50x50_snowshowers_day;
    } else {
      icon = met_bitmap_black_50x50_snowshowers_night;
    }
  }
  
  // 6. МОКРЫЙ СНЕГ / SLEET SHOWERS
  else if (condition.indexOf("showers") >= 0 && condition.indexOf("sleet") >= 0) {
    if (day) {
      icon = met_bitmap_black_50x50_sleetshowers_day;
    } else {
      icon = met_bitmap_black_50x50_sleetshowers_night;
    }
  }
  
  // 7. ГРОЗА С ОСАДКАМИ / THUNDERSTORM
  else if (condition.indexOf("thunderstorm") >= 0 || 
           (condition.indexOf("thunder") >= 0 && condition.indexOf("rain") >= 0)) {
    if (day) {
      icon = met_bitmap_black_50x50_heavysleetshowersandthunder_day;
    } else {
      icon = met_bitmap_black_50x50_heavyrainshowersandthunder_night;
    }
  }
  
  // 8. ОБЫЧНЫЙ ДОЖДЬ / RAIN / DRIZZLE
  else if (condition.indexOf("rain") >= 0 || condition.indexOf("drizzle") >= 0) {
    icon = met_bitmap_black_50x50_rain;
  }
  
  // 9. СНЕГ / SNOW / BLIZZARD
  else if (condition.indexOf("snow") >= 0 || condition.indexOf("blizzard") >= 0) {
    icon = met_bitmap_black_50x50_snow;
  }
  
  // 10. МОКРЫЙ СНЕГ / SLEET
  else if (condition.indexOf("sleet") >= 0) {
    icon = met_bitmap_black_50x50_sleet;
  }
  
  // 11. ГРОЗА БЕЗ ОСАДКОВ / THUNDER
  else if (condition.indexOf("thunder") >= 0 || condition.indexOf("storm") >= 0) {
    icon = met_bitmap_black_50x50_lightrainandthunder;
  }
  
  // 12. ТУМАН / FOG / MIST
  else if (condition.indexOf("fog") >= 0 || condition.indexOf("mist") >= 0 || condition.indexOf("haze") >= 0) {
    icon = met_bitmap_black_50x50_fog;
  }
  
  // 13. ВЕТЕР / WIND
  else if (condition.indexOf("wind") >= 0) {
    icon = met_bitmap_black_50x50_clearsky_polartwilight;
  }
  
  // 14. ПО УМОЛЧАНИЮ - ясно
  else {
    icon = day ? met_bitmap_black_50x50_clearsky_day : met_bitmap_black_50x50_clearsky_night;
  }
  
  // Отрисовка XBM битмапа 50x50
  if (icon != nullptr) {
    tft.drawBitmap(x, y, icon, ICON_WIDTH, ICON_HEIGHT, color);
  }
}


// === ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ: центрирование иконки ===
void drawWeatherIconCentered(int x, int y, String condition, uint16_t color) {
  // Центрируем относительно точки (x, y)
  drawWeatherIcon(x - ICON_WIDTH/2, y - ICON_HEIGHT/2, condition, color);
}

