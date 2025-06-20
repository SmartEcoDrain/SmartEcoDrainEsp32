#include "configs.h"
#include "Utils/utilities.h"
#include "Utils/device_id.h"
#include "Setup/device_setup.h"
#include "Database/device_db.h"
#include "Database/address.h"

// #define DUMP_AT_COMMANDS


#ifdef TINY_GSM_MODEM_A7670
#undef TINY_GSM_MODEM_A7670
#endif

#ifdef TINY_GSM_MODEM_A7608
#undef TINY_GSM_MODEM_A7608
#endif

#ifndef TINY_GSM_MODEM_A76XXSSL
#define TINY_GSM_MODEM_A76XXSSL // Support A7670X/A7608X/SIM7670G
#endif

#define TINY_GSM_DEBUG Serial

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

// Create modem and HTTP clients
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClientSecure client(modem);

// Global objects
DeviceSetup* deviceSetup = nullptr;
DeviceDB* deviceDB = nullptr;
AddressDB* addressDB = nullptr;
String deviceId;

// Function declarations
bool initializeModem();
void testApiConnection();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Smart Echo Drain ESP32 Starting ===");
  Serial.printf("Device Version: %s\n", DEVICE_VERSION);
  Serial.printf("API Key: %s\n", ESP32_API_KEY);
  
  // Generate or get device ID
  deviceId = getOrGenerateDeviceId();
  Serial.printf("Device ID: %s\n", deviceId.c_str());

  // Initialize device setup
  deviceSetup = new DeviceSetup(deviceId, &modem, &client);

  // Check if device setup is already completed
  if (deviceSetup->isSetupCompleted()) {
    Serial.println("Device setup already completed. Initializing normal operation...");
    
    // Initialize modem for normal operation
    if (initializeModem()) {
      // Initialize database connections
      deviceDB = new DeviceDB(&modem, &client);
      addressDB = new AddressDB(&modem, &client);
      
      // Test API connection
      testApiConnection();
      
      // Check if device exists in database
      if (deviceSetup->checkDeviceSetupStatus()) {
        Serial.println("✓ Device verified in database");
        Serial.printf("Device Name: %s\n", deviceSetup->getDeviceName().c_str());
        Serial.printf("Location: %s\n", deviceSetup->getDeviceLocation().c_str());
        
        // Send initial heartbeat
        int heartbeatResult = deviceDB->sendHeartbeat(deviceId);
        Serial.printf("Initial heartbeat sent - Status: %d\n", heartbeatResult);
      } else {
        Serial.println("Device not found in database. Starting setup mode...");
        deviceSetup->resetSetupFlag();
        deviceSetup->startSetupMode();
      }
    } else {
      Serial.println("Failed to initialize modem. Starting setup mode...");
      deviceSetup->startSetupMode();
    }
  } else {
    Serial.println("Device not configured. Starting setup mode...");
    deviceSetup->startSetupMode();
  }
}

void loop() {
  if (deviceSetup && deviceSetup->isInSetupMode()) {
    // Handle setup mode
    deviceSetup->loop();
  } else {
    // Normal operation mode
    static unsigned long lastHeartbeat = 0;
    static unsigned long lastDataSend = 0;
    
    // Send heartbeat every 30 seconds
    if (millis() - lastHeartbeat > 30000) {
      if (deviceDB) {
        int result = deviceDB->sendHeartbeat(deviceId);
        Serial.printf("Heartbeat sent - Status: %d\n", result);
      }
      lastHeartbeat = millis();
    }
    
    // Send sensor data every 5 minutes (for now just test data)
    if (millis() - lastDataSend > 300000) {
      if (deviceDB) {
        // Create test device data
        DeviceData testData;
        testData.deviceId = deviceId;
        // testData.timestamp = millis();
        // testData.waterLevel = random(0, 100);
        testData.turbidity = random(0, 1000);
        testData.weight = random(0, 50);
        testData.ultrasonic = random(5, 200);
        
        // Add some module status data
        testData.moduleStatus["sensor"] = "online";
        testData.moduleStatus["connectivity"] = "good";
        testData.moduleOtherData["battery"] = String(random(70, 100));
        testData.moduleOtherData["temperature"] = String(random(25, 35));
        
        int result = deviceDB->createDeviceData(testData);
        Serial.printf("Sensor data sent - Status: %d\n", result);
      }
      lastDataSend = millis();
    }
    
    delay(1000); // Small delay to prevent overwhelming the system
  }
}

bool initializeModem() {
  Serial.println("\n=== Initializing Modem ===");
  
  // Initialize Serial for modem
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

#ifdef BOARD_POWERON_PIN
  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);
  Serial.println("Modem power enabled");
#endif

#ifdef MODEM_RESET_PIN
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  Serial.println("Modem reset completed");
#endif

  // Power key sequence
  pinMode(BOARD_PWRKEY_PIN, OUTPUT);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, HIGH);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);
  
  Serial.println("Starting modem initialization...");

  // Test modem communication
  if (!modem.testAT(10000)) {
    Serial.println("Failed to connect to modem");
    return false;
  }
  Serial.println("Modem connection established");

  // Check SIM card
  SimStatus sim = modem.getSimStatus();
  if (sim != SIM_READY) {
    Serial.println("SIM card not ready");
    return false;
  }
  Serial.println("SIM card ready");

  // Wait for network registration
  if (!modem.waitForNetwork(60000)) {
    Serial.println("Failed to connect to network");
    return false;
  }
  Serial.println("Connected to cellular network");

  // Connect to GPRS
  if (!modem.gprsConnect("internet")) { // Use your APN here
    Serial.println("Failed to connect to GPRS");
    return false;
  }
  Serial.println("GPRS connected");

  String ipAddress = modem.getLocalIP();
  Serial.printf("Modem IP: %s\n", ipAddress.c_str());
  
  return true;
}

void testApiConnection() {
  Serial.println("\n=== Testing API Connection ===");
  
  if (!addressDB) {
    Serial.println("AddressDB not initialized");
    return;
  }
  
  // Test address API
  Serial.println("Testing address API...");
  String regions = addressDB->getRegions();
  
  if (!regions.isEmpty() && regions != "[]" && regions != "{}") {
    Serial.println("✓ Address API connection successful");
    Serial.printf("Regions data length: %d bytes\n", regions.length());
    
    // Parse and show first few regions
    JsonDocument doc;
    deserializeJson(doc, regions);
    if (doc.is<JsonArray>() && doc.size() > 0) {
      Serial.printf("Found %d regions in database\n", doc.size());
      for (int i = 0; i < min(3, (int)doc.size()); i++) {
        JsonObject region = doc[i];
        Serial.printf("  - %s: %s\n", 
                      region["reg_code"].as<String>().c_str(),
                      region["reg_desc"].as<String>().c_str());
      }
    }
  } else {
    Serial.println("✗ Address API connection failed");
    Serial.println("Response: " + regions);
  }
  
  // Test device API if deviceDB is available
  if (deviceDB) {
    Serial.println("Testing device API...");
    String deviceInfo = deviceDB->getDevice(deviceId);
    
    if (!deviceInfo.isEmpty() && deviceInfo != "{}" && deviceInfo.indexOf("error") == -1) {
      Serial.println("✓ Device API connection successful");
    } else {
      Serial.println("✗ Device API connection failed or device not found");
      Serial.println("Response: " + deviceInfo);
    }
  }
}