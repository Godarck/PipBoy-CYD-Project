# PipBoy-CYD-Project
PipBoy arduino project on 2.4 inch ESP32-2432S024R with Resistive touch


* ESP32 2.4" TFT ILI9341 + XPT2046 Touch + RTC DS1307 + AT24C32 EEPROM + WiFi
 * Стилизованный Pip-Boy из Fallout
  

* Flash settings:
  
 Flash mode: DIO
 
 Partition scheme: NoOta 2mb APP / 2mb SPIFFS
 
 Events run: core1
 
 Arduino runs: core1
 
 Upload speed: 460800
 
 PSRAm: Enabled
 

*  libs :

 //esp32-audioi2s-master
 
 esp8266audio

 time from Michael Margolis

 HTTPClient

 ArduinoJson
 // ???? not use now - for ogg lib https://github.com/pschatzmann/codec-ogg


* ====== Settings: =========
 Password default for wifi in wifi_module.cpp

 Default folder from SD card for mp3 in RadioModule.cpp
 
 In RadioModule.cpp pin 26 for output audio stream


* ====== components =========
 
 * CYD: 2.4 inch ESP32-2432S024R with Resistive touch (Only type-c usb connector, RGB led in Front near display. )
 * RTC + EEPROM :   Tyni RTC I2C module DS1307 with EEPROM
 * connectors:       JST 1.25 4pin - for i2c bus
                  JST 1.25 2pin (2 pcs)  - for dinamic and battery
   
 Для работы часов нужен модульс флэшкой памяти (на модуле должно быть две 8 ногих микросхемы)/ либо флэшка памяти отдельно i2c
 

 * Dinamic динамик ( можно от мобильника, например с старых айфонов)
 * модуль сенсора пульса (опционально) пока не реализовано
 * флэшка microSD (до 32 гб, чем меньше тем лучше. оптимально - 8 гб) FAT32. С нее считывается музыка локально. Радиостанции из фалаут можно найти отдельно.


======= настройки и работа ======

пароль вай вай запоминает последний введенный,

координаты GPS и выбор температры( Celsius - Farengheit) запоминает последние введенные

папку с MP3 файлами так же запоминает введенную (по умолчанию SD:/mp3/)

====== на главном экране ======
* слева в углу 3 состояния:
 в рамке - активно
 * wifi - подключен ли WiFi
 * Rad - идет ли воспроизведение музыки
 * W индикатор погоды:
   W Err - данных о погоде нет
   
   W [E] - данные считанные из EEPROM
   
   W [O] - Данные актуальные из OpenMeteo
   
   W [W] - Данные актуальные из WTTR


HP - индикатор пульса (not realize)

HP - зависит от времени суток (с 8 утра до 8 вечера уменьшается с 420 до 80) Потом восстанавливается до максимума к 03:00

AP - зависит от времени суток (с 8 утра до 13:00 уменьшается до 60, потом до 5:00 уменьшается до 30.
 Потом с 5:00 до 5:30 уменьшается до 0 каждую минуту отнимаеся 1) Потом восстанавливается до максимума каждые 5 минут.




