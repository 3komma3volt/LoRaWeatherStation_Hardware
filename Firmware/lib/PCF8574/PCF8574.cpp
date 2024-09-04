#include <Wire.h>
#include "PCF8574.h"

void PCF8574_Write(uint8_t address, uint8_t data) {
  Wire.beginTransmission(address);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t PCF8574_Read(uint8_t address) {
  uint8_t data=0xff;
  Wire.requestFrom(address,1);
  data = Wire.read();
  Wire.endTransmission();

  return data;
}

uint8_t PCF8574_Init(uint8_t address) {
  Wire.begin();
  Wire.beginTransmission(address);
  return !Wire.endTransmission();
}