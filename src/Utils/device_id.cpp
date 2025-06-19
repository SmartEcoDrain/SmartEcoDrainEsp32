#include "device_id.h"
#include <EEPROM.h>
#include <esp_random.h>

String generateUUID() {
  String uuid = "";
  
  for (int i = 0; i < 32; i++) {
    if (i == 8 || i == 12 || i == 16 || i == 20) {
      uuid += "-";
    }
    
    uint8_t randomByte = esp_random() & 0xFF;
    if (randomByte < 16) {
      uuid += "0";
    }
    uuid += String(randomByte, HEX);
  }
  
  return uuid;
}

String getOrGenerateDeviceId() {
  EEPROM.begin(512);
  
  // Check if device ID exists
  char deviceIdBuffer[DEVICE_ID_LENGTH + 1];
  bool hasValidId = true;
  
  for (int i = 0; i < DEVICE_ID_LENGTH; i++) {
    deviceIdBuffer[i] = EEPROM.read(DEVICE_ID_ADDR + i);
    if (deviceIdBuffer[i] == 0xFF || deviceIdBuffer[i] == 0x00) {
      hasValidId = false;
      break;
    }
  }
  deviceIdBuffer[DEVICE_ID_LENGTH] = '\0';
  
  String deviceId;
  
  if (hasValidId && strlen(deviceIdBuffer) == DEVICE_ID_LENGTH) {
    deviceId = String(deviceIdBuffer);
    Serial.println("Loaded existing Device ID: " + deviceId);
  } else {
    // Generate new UUID
    deviceId = generateUUID();
    Serial.println("Generated new Device ID: " + deviceId);
    
    // Store in EEPROM
    for (int i = 0; i < DEVICE_ID_LENGTH; i++) {
      EEPROM.write(DEVICE_ID_ADDR + i, deviceId[i]);
    }
    EEPROM.commit();
  }
  
  EEPROM.end();
  return deviceId;
}

void generateNewDeviceId() {
  String newId = generateUUID();
  
  EEPROM.begin(512);
  for (int i = 0; i < DEVICE_ID_LENGTH; i++) {
    EEPROM.write(DEVICE_ID_ADDR + i, newId[i]);
  }
  EEPROM.commit();
  EEPROM.end();
  
  Serial.println("Generated and stored new Device ID: " + newId);
}