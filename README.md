* CYD ESP32-2432S024R 2.4" 320x240 TFT ILI9341 + Resisteve Touch
* CYD ESP32-3248S035R 3.5" 480x320 TFT ST7796  + Resisteve Touch

* 
  Modules:

  RTC DS1307 with EEPROM or
  AT24C32 (AT24C64, AT24C256) EEPROM
  
  /OPTIONAL/ GPS NEO-6M or simmular (3 different connections: UART RX-TX, i2c bridge SC16IS750, ESP32C3SuperMini)

  /OPTIONAL/ BMP280 or BME280 (if you need humidity)

  /OPTIONAL/ GY-MAX30102 Pulse and ox meter

  /OPTIONAL/ TOF10120 laser meter

  /OPTIONAL GYRO in progress/

  /OPTIONAL MAGNITOMETR in progress/

  
* Stylized Pip-Boy from Fallout

  ![image](https://github.com/Godarck/PipBoy-CYD-Project/blob/main/Photo/IMG_20260422_123901.png)
  

* ======== !!! ARDUINO IDE PREFERENCES: !!!  ========
  
   ===> ESP32 DEV Module - COM Port USB
  
Flash mod: DIO

Flash frequency: 80Mhz

Partition scheme: NoOta 2mb APP / 2mb SPIFFS

Events run: core1

Arduino runs: core1

Upload speed: 460800 (choose max speed)

PSRAM: Disabled 


* ======== Boards: ======== 

  ESP32 by Espressif Systems          ver 3.3.7


* ======== Libraries: ======== 
  
  RTClib by Adafruit                    ver 2.1.4
  
  Timezone Library by Jack Christensen  ver 1.2.6
  
  ESP8266Audio by Early F.Philhover     ver 2.4.1
  
  Time by Michael Margolis              ver 1.6.1 
  
  ArduinoJson by Benoit Blanchon        ver 7.4.3 
  
  TFT_eSPI by Boodmer                   ver 2.5.43 (Copy user_setup.h to TFT_eSPI lib folder)
  
  Adafruit BusIO by Adafruit            ver 1.17.4
  
  Adafruit BMP280 Library by Adafruit   ver 3.0.0
  
  Adafruit BME280 Library by Adafruit   ver 2.3.0
  
  PNGDec by Larry Bank                  ver 1.1.6
  
  SparkFun MAX3010x pulse and Proximity ver 1.1.2
  
  TinyGPSPlus by Mikal Hart             ver 0.0.4 

* ======== Configure: ======== 
  
    Config in config.h AND UserSetup.h (TFT_eSPI library)
  
  //#define CYD2_4        // Uncomment if you use 2.4 CYD
  
  //#define CYD3_5          // Uncomment if you use 3.5 CYD

    Config in config.h 
  
  PERSON_NAME - Name in main screen
  
  DEBUGFLAG - if you need debug. Set flag true for first run. Use Serial monitor to watch Debug information.
  
  GMT_SET - to set GMT region for clock
  
  Default Password for WiFI in var StandartWiFiPass in main *.ino file  (only for first use)
  
  Default folder from SD card for mp3 in RadioModule.cpp  (only for first use)



* ======== Components =========

  => CYD: 2.4 inch ESP32-2432S024R with Resistive Touch (Only Type-C USB connector, RGB LED in front near display.)

  => CYD ESP32-3248S035R 3.5" 480x320 TFT ST7796  + Resisteve Touch
  
  RTC + EEPROM: Tyni RTC I2C module DS1307
  
  Connectors:

    JST 1.25 4-pin - for I2C bus
  
    JST 1.25 2-pin (2 pcs) - for speaker and battery
  
    FPC 0.5mm 6p - for GPS

For the watch to work, a module with flash memory is required (the module must have two 8-pin chips) or a separate I2C flash memory card.


Speaker (can be from a mobile phone, for example, from old iPhones)

Heart rate sensor module (optional) not yet implemented

MicroSD flash card (up to 32 GB, the smaller is the better. 8 GB is optimal) FAT32. Music is read from it locally. Fallout radio stations can be found separately.


* ======= Features ======

Integrated I2C scanner in start up initialize.

Wi-Fi password remembers the last entered one,

GPS coordinates and temperature selection (Celsius - Fahrenheit) remember the last entered ones,

MP3 folder also remembers the entered one (SD:/mp3/ by default)


Clock function. Time synchronization via WiFi

Background radio playback via WiFi or from an SD card

3 timers counting down in minutes to a specified time (stylistically displayed on the right side of the main screen as a list of items and their quantity)

Several character status images for the main screen

Weather display function for specified location coordinates

Wi-Fi functions: scanning, list of networks, connecting to a selected one (must always be connected manually)

HP and AP indicators are calculated automatically, depending on the time of day. During the day, they decrease. After evening, they replenish.


* ====== On the main screen ======

Left corner: 3 states:

In frame - active

Wi-Fi - WiFi connected

Rad - music playing

W weather indicator:

W Err - no weather data

W [E] - data read from EEPROM

W [O] - current data from OpenMeteo

W [W] - current data from WTTR


Right (limited to three lines): list of items (configured in general - time)

Displays the number of minutes left until a certain timer (in minutes, limited to 240)

Line structure: (minutes remaining) name


HP - stylized heart rate indicator

HP - depends on the time of day (from 8 AM to 8 PM it decreases from 420 to 80). It then returns to its maximum at 3:00 AM

AP - depends on the time of day (from 8 AM to 1 PM it decreases to 60, then decreases to 30 by 5:00 AM). Then from 5:00 to 5:30 it 
decreases to 0 every minute by 1) Then it is restored to the maximum every 5 minutes.
