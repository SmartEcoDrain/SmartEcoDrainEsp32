#define TINY_GSM_RX_BUFFER 1024
// #define DUMP_AT_COMMANDS

#include "configs.h"
#include "Utils/utilities.h"
#include "Utils/device_id.h"
#include "Setup/device_setup.h"
#include "Database/device_db.h"
#include "Database/user_db.h"
#include "Database/address.h"
#include <ArduinoJson.h>

// Esp32 specific includes
#include <EEPROM.h>

// Include TinyGSM Supabase library
#include <TinyGsmClient.h>
#include <TinyGSMSupabase/supabase.h>

// Module libraries
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
Supabase db;

// Global variables
String deviceId;
DeviceSetup* deviceSetup;
DeviceDB* deviceDB;
UserDB* userDB;
AddressDB* addressDB;

// Sensor data structure with separate device and module status
struct SensorData {
  // Device data
  float cpuTemperature;
  int signalStrength;
  float batteryVoltage;
  String firmwareVersion;
  unsigned long uptime;
  
  // Module data
  float tof1;
  float tof2;
  float force1;
  float force2;
  float weight1;
  float weight2;
  float turbidity1;
  float turbidity2;
  float ultrasonic1;
  float ultrasonic2;
  
  // Separate status tracking
  struct {
    String status;
    String message;
  } device;
  
  struct {
    String status;
    String message;
  } module;
};

void initializeModem()
{
  Serial.begin(115200);
  Serial.println("=== TinyGSM Supabase Data Logger ===");
  Serial.printf("Device ID: %s\n", deviceId.c_str());
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

#ifdef BOARD_POWERON_PIN
  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif

#ifdef MODEM_RESET_PIN
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
#endif

  pinMode(BOARD_PWRKEY_PIN, OUTPUT);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, HIGH);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);

  Serial.println("Starting modem...");

  int retry = 0;
  while (!modem.testAT(1000))
  {
    Serial.print(".");
    if (retry++ > 10)
    {
      digitalWrite(BOARD_PWRKEY_PIN, LOW);
      delay(100);
      digitalWrite(BOARD_PWRKEY_PIN, HIGH);
      delay(1000);
      digitalWrite(BOARD_PWRKEY_PIN, LOW);
      retry = 0;
    }
  }
  Serial.println(" Modem ready!");

  // Check SIM card
  SimStatus sim = SIM_ERROR;
  while (sim != SIM_READY)
  {
    sim = modem.getSimStatus();
    switch (sim)
    {
    case SIM_READY:
      Serial.println("SIM card online");
      break;
    case SIM_LOCKED:
      Serial.println("SIM card locked. Please unlock first.");
      break;
    default:
      Serial.println("SIM card not ready");
      break;
    }
    delay(1000);
  }

#ifndef TINY_GSM_MODEM_SIM7672
  if (!modem.setNetworkMode(MODEM_NETWORK_AUTO))
  {
    Serial.println("Set network mode failed!");
  }
#endif

  // Wait for network registration
  Serial.print("Waiting for network registration...");
  RegStatus status = REG_NO_RESULT;
  while (status == REG_NO_RESULT || status == REG_SEARCHING || status == REG_UNREGISTERED)
  {
    status = modem.getRegistrationStatus();
    switch (status)
    {
    case REG_UNREGISTERED:
    case REG_SEARCHING:
      Serial.print(".");
      delay(1000);
      break;
    case REG_DENIED:
      Serial.println("Network registration denied!");
      return;
    case REG_OK_HOME:
      Serial.println("Registered on home network");
      break;
    case REG_OK_ROAMING:
      Serial.println("Registered on roaming network");
      break;
    default:
      delay(1000);
      break;
    }
  }

  if (!modem.setNetworkActive())
  {
    Serial.println("Enable network failed!");
  }

  String ipAddress = modem.getLocalIP();
  Serial.print("Local IP: ");
  Serial.println(ipAddress);

  Serial.println("Modem initialization complete!");
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== Device Starting ===");
  
  // Get or generate device ID
  deviceId = getOrGenerateDeviceId();
  
  // Initialize database classes
  deviceSetup = new DeviceSetup(deviceId, &db);
  
  // Check if setup is completed locally
  bool setupCompleted = deviceSetup->isSetupCompleted();
  
  if (!setupCompleted) {
    Serial.println("Setup required - Starting setup mode...");
    deviceSetup->startSetupMode();
    return;
  }
  
  Serial.println("Setup completed - Starting normal operation...");
  
  // Initialize modem and network
  initializeModem();
  
  // Initialize Supabase
  Serial.println("Initializing Supabase connection...");
  db.begin(SUPABASE_URL, SUPABASE_ANON_KEY, modem, client);
  
  // Initialize database classes
  deviceDB = new DeviceDB(&db);
  userDB = new UserDB(&db);
  addressDB = new AddressDB(&db);
  
  // Check setup status in database
  if (!deviceSetup->checkDeviceSetupStatus()) {
    Serial.println("Database setup incomplete - requires online setup");
    // Could enter setup mode again or continue with default values
  }
  
  Serial.println("=== Setup Complete ===");
  Serial.println("Starting data logging loop...\n");
  Serial.printf("Device %s (%s) is now operational\n", 
                deviceId.c_str(), 
                deviceSetup->getDeviceName().c_str());
}

void loop()
{
  // Handle setup mode
  if (deviceSetup->isInSetupMode()) {
    deviceSetup->loop();
    return;
  }
  
  static unsigned long lastDataLog = 0;
  static unsigned long lastStatusPrint = 0;
  const unsigned long DATA_LOG_INTERVAL = 60000;  // Log data every 60 seconds
  const unsigned long STATUS_PRINT_INTERVAL = 10000; // Print status every 10 seconds
  
  unsigned long currentTime = millis();
  
  // Print status periodically
  if (currentTime - lastStatusPrint >= STATUS_PRINT_INTERVAL)
  {
    Serial.printf("[%lu] %s (%s) running... Next data log in %lu seconds\n", 
                  currentTime / 1000, 
                  deviceId.c_str(),
                  deviceSetup->getDeviceName().c_str(),
                  (DATA_LOG_INTERVAL - (currentTime - lastDataLog)) / 1000);
    lastStatusPrint = currentTime;
  }
  
  // Log data periodically (this will trigger the online status update)
  if (currentTime - lastDataLog >= DATA_LOG_INTERVAL)
  {
    // Check network connection
    if (!modem.isNetworkConnected())
    {
      Serial.println("Network disconnected! Attempting reconnection...");
      initializeModem();
      delay(5000);
      return;
    }
    
    // Example of updating device status
    deviceDB->updateDeviceStatus(deviceId, true);
    
    lastDataLog = currentTime;
  }
  
  // Small delay to prevent busy waiting
  delay(100);
}