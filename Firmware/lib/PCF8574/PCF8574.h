
#ifndef PCF8574_h
#define PCF8574_h

#include <Arduino.h>

#define PCF8574 0x20


void PCF8574_Write(uint8_t address, uint8_t data);
uint8_t PCF8574_Read(uint8_t address);
uint8_t PCF8574_Init(uint8_t address) ;

#endif