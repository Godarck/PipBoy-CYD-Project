#include "eeprom_module.h"
#include "config.h"

 bool eepromFound = false;

void eepromInit() {
  Wire.beginTransmission(EEPROM_ADDRESS);
  if (Wire.endTransmission() == 0) {
    eepromFound = true;
    if (DEBUGFLAG) Serial.println("[EEPROM] AT24C32     OK");
  } else {
    if (DEBUGFLAG) Serial.println("[EEPROM] AT24C32     ERROR = not found");
  }
}

bool eepromIsFound() {
  return eepromFound;
}

bool eepromWriteSlot(uint8_t slot, uint8_t* data) {
  if (slot > 127 || !eepromFound) return false;
  
  uint16_t address = slot * EEPROM_PAGE_SIZE;
  
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((address >> 8) & 0xFF);
  Wire.write(address & 0xFF);

  if (DEBUGFLAG) Serial.println("[EEPROM] Write in EEPROM");
  for (int i = 0; i < 32; i++) {
    Wire.write(data[i]);
    if (DEBUGFLAG) Serial.print(" ");
    if (DEBUGFLAG) Serial.print(data[i]);
  }
  if (DEBUGFLAG) Serial.println(" ");
  uint8_t result = Wire.endTransmission();
  delay(10);  // AT24C32 требует 10ms на запись
  
  return (result == 0);
}


bool eepromReadSlot(uint8_t slot, uint8_t* buffer) {
  if (slot > 127 || !eepromFound) return false;
  
  uint16_t address = slot * EEPROM_PAGE_SIZE;
  
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((address >> 8) & 0xFF);
  Wire.write(address & 0xFF);
  
  if (Wire.endTransmission(false) != 0) return false;
  
  uint8_t got = Wire.requestFrom(EEPROM_ADDRESS, 32);
  if (got != 32) {
    if (DEBUGFLAG) Serial.printf("[EEPROM] read slot %d ERROR: got %d bytes instead of 32", slot, got);
    return false;
  }

  if (DEBUGFLAG) Serial.printf("[EEPROM] READ EEPROM from slot %d OK :\n", slot);
  for (int i = 0; i < 32; i++) {
    buffer[i] = Wire.available() ? Wire.read() : 0xFF;
    if (DEBUGFLAG) Serial.print(buffer[i], HEX);
    if (DEBUGFLAG) Serial.print(" ");
    if ((i + 1) % 16 == 0) if (DEBUGFLAG) Serial.println();
  }
  bool isEmpty = true;
  for (int i = 0; i < 32; i++) {
    if (buffer[i] != 0xFF)
    {
      isEmpty = false;
      break;
    }
  }

  if (isEmpty)
  {
     if (DEBUGFLAG) Serial.printf("[EEPROM] READ EEPROM from slot %d  OK : \nSLOT EMPTY", slot);
    return false;
  }
  if (DEBUGFLAG) Serial.println();
  return true;
}

