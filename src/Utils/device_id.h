#ifndef DEVICE_ID_H
#define DEVICE_ID_H

#include <Arduino.h>

// EEPROM addresses for device ID storage
#define DEVICE_ID_ADDR 100
#define DEVICE_ID_LENGTH 36  // UUID length

String getOrGenerateDeviceId();
void generateNewDeviceId();
String generateUUID();

#endif