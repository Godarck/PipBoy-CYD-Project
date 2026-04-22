* CYD ESP32-2432S024R 2.4" TFT ILI9341 + XPT2046 Touch + RTC DS1307 + AT24C32 EEPROM + WiFi
  
* Stylized Pip-Boy from Fallout

  

* ARDUINO IDE PREFERENCES:
  
Flash mod: DIO

partition scheme: NoOta 2mb APP / 2mb SPIFFS

events run: core1

arduino runs: core1

Upload speed: 460800

PSRAM: Enabled 


libs:

esp8266audio

time from Michael Margolis

HTTPClient

Arduino Json


password default for wifi in wifi_module.cpp

Default folder from SD card for mp3 in RadioModule.cpp

in RadioModule.cpp pin 26 for output audio stream


* ======= Components =========

CYD: 2.4 inch ESP32-2432S024R with Resistive Touch (Only Type-C USB connector, RGB LED in front near display.)

RTC + EEPROM: Tyni RTC I2C module DS1307

Connectors: JST 1.25 4-pin - for I2C bus

JST 1.25 2-pin (2 pcs) - for speaker and battery

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


* ====== on the main screen ======

Left corner: 3 states:

framed - active

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


HP - heart rate indicator

HP - depends on the time of day (from 8 AM to 8 PM it decreases from 420 to 80). It then returns to its maximum at 3:00 AM

AP - depends on the time of day (from 8 AM to 1 PM it decreases to 60, then decreases to 30 by 5:00 AM). Then from 5:00 to 5:30 it 
decreases to 0 every minute by 1) Then it is restored to the maximum every 5 minutes.
