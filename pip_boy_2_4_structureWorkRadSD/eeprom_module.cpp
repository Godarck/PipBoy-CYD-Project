#include "eeprom_module.h"

 bool eepromFound = false;

void eepromInit() {
  Wire.beginTransmission(EEPROM_ADDRESS);
  if (Wire.endTransmission() == 0) {
    eepromFound = true;
    Serial.println("EEPROM AT24C32 OK");
  } else {
    Serial.println("EEPROM not found");
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

  Serial.println("Write in EEPROM");
  for (int i = 0; i < 32; i++) {
    Wire.write(data[i]);
    Serial.print(" ");
    Serial.print(data[i]);
  }
  Serial.println(" ");
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
    Serial.print("EEPROM read error: got ");
    Serial.print(got);
    Serial.println(" bytes instead of 32");
    return false;
  }

  Serial.println("READ from EEPROM:");
  for (int i = 0; i < 32; i++) {
    buffer[i] = Wire.available() ? Wire.read() : 0xFF;
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
    if ((i + 1) % 16 == 0) Serial.println();
  }
  bool isEmpty = true;
  for (int i = 0; i < 32; i++) {
    if (buffer[i] != 0xFF)
    {
      isEmpty = false;
      break;
    }
  }
    Serial.println();
  if (isEmpty)
    return false;

  return true;
}

