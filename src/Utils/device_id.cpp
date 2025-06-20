#include "device_id.h"
#include <EEPROM.h>
#include <esp_random.h>

String generateUUID() {
  String uuid = "";
  
  for (int i = 0; i < 16; i++) {
    uint8_t randomByte = esp_random() & 0xFF;
    
    // Add hyphens at the correct positions (after 4, 6, 8, 10 bytes)
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      uuid += "-";
    }
    
    // Format byte as 2-digit hex
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