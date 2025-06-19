#define TINY_GSM_RX_BUFFER 1024
#define DUMP_AT_COMMANDS

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

// Sensor instances
Adafruit_VL53L0X vl53l0x = Adafruit_VL53L0X();

// Sensor data structure with separate device and module status
struct SensorData {
  // Device data
  float cpuTemperature;
  float cpuFrequency;
  float ramUsage;
  float storageUsage;
  int signalStrength;
  float batteryVoltage;
  float batteryPercentage;
  float solarWattage;
  unsigned long uptime;
  String batteryStatus;
  String firmwareVersion;
  bool isOnline;
  
  // Module data
  float tof;
  float force0;
  float force1;
  float weight;
  float turbidity;
  float ultrasonic;
  
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

void initializeSensors() {
  Serial.println("Initializing sensors...");
  
  Wire.begin();
  
  // Initialize VL53L0X TOF sensor
  if (!vl53l0x.begin()) {
    Serial.println("Failed to boot VL53L0X");
  } else {
    Serial.println("VL53L0X sensor initialized");
  }
  
  // Initialize other sensors here
  Serial.println("Sensor initialization complete!");
}

SensorData readSensorData() {
  SensorData data;
  
  // Read device data
  data.cpuTemperature = temperatureRead();
  data.cpuFrequency = ESP.getCpuFreqMHz();
  data.ramUsage = (ESP.getHeapSize() - ESP.getFreeHeap()) / (float)ESP.getHeapSize() * 100.0;
  data.storageUsage = 0; // TODO: Calculate storage usage
  data.signalStrength = modem.getSignalQuality();
  data.batteryVoltage = 3.7; // TODO: Read actual battery voltage
  data.batteryPercentage = 85; // TODO: Calculate battery percentage
  data.solarWattage = 0; // TODO: Read solar panel wattage
  data.uptime = millis();
  data.batteryStatus = "NORMAL";
  data.firmwareVersion = DEVICE_VERSION;
  data.isOnline = modem.isNetworkConnected();
  
  // Set device status based on readings
  if (data.cpuTemperature > 80) {
    data.device.status = "WARNING";
    data.device.message = "High CPU temperature detected";
  } else if (data.batteryPercentage < 20) {
    data.device.status = "WARNING";
    data.device.message = "Low battery level";
  } else if (!data.isOnline) {
    data.device.status = "ERROR";
    data.device.message = "Network disconnected";
  } else {
    data.device.status = "NORMAL";
    data.device.message = "All systems operational";
  }
  
  // Read module data
  VL53L0X_RangingMeasurementData_t measure;
  vl53l0x.rangingTest(&measure, false);
  
  if (measure.RangeStatus != 4) {
    data.tof = measure.RangeMilliMeter;
    data.module.status = "NORMAL";
    data.module.message = "Sensor readings normal";
  } else {
    data.tof = -1;
    data.module.status = "ERROR";
    data.module.message = "TOF sensor out of range";
  }
  
  // TODO: Read other sensor values
  data.force0 = random(0, 1000) / 10.0; // Dummy data
  data.force1 = random(0, 1000) / 10.0; // Dummy data
  data.weight = random(0, 5000) / 10.0; // Dummy data
  data.turbidity = random(0, 1000) / 10.0; // Dummy data
  data.ultrasonic = random(50, 500); // Dummy data
  
  return data;
}

String getTemperatureStatus(float temp) {
  if (temp < 0) {
    return "COLD";
  } else if (temp < 30) {
    return "NORMAL";
  } else if (temp < 60) {
    return "WARM";
  } else if (temp < 85) {
    return "HOT";
  } else {
    return "CRITICAL";
  }
}

void logDeviceData(const SensorData& sensorData) {
  Serial.println("\n=== Logging Device Data ===");
  
  // Create DeviceData structure
  DeviceData deviceData;
  deviceData.deviceId = deviceId;
  
  // Fill device data
  deviceData.device.cpuTemperature = sensorData.cpuTemperature;
  deviceData.device.cpuFrequency = sensorData.cpuFrequency;
  deviceData.device.ramUsage = sensorData.ramUsage;
  deviceData.device.storageUsage = sensorData.storageUsage;
  deviceData.device.signalStrength = sensorData.signalStrength;
  deviceData.device.batteryVoltage = sensorData.batteryVoltage;
  deviceData.device.batteryPercentage = sensorData.batteryPercentage;
  deviceData.device.solarWattage = sensorData.solarWattage;
  deviceData.device.uptimeMs = sensorData.uptime;
  deviceData.device.batteryStatus = sensorData.batteryStatus;
  deviceData.device.isOnline = sensorData.isOnline;
  
  // Add device status
  deviceData.device.status["status"] = sensorData.device.status;
  deviceData.device.status["message"] = sensorData.device.message;
  deviceData.device.status["firmware"] = sensorData.firmwareVersion;
  deviceData.device.status["temperature_status"] = getTemperatureStatus(sensorData.cpuTemperature);
  
  // Fill module data
  deviceData.module.tof = sensorData.tof;
  deviceData.module.force0 = sensorData.force0;
  deviceData.module.force1 = sensorData.force1;
  deviceData.module.weight = sensorData.weight;
  deviceData.module.turbidity = sensorData.turbidity;
  deviceData.module.ultrasonic = sensorData.ultrasonic;
  
  // Add module status
  deviceData.module.status["status"] = sensorData.module.status;
  deviceData.module.status["message"] = sensorData.module.message;
  
  // Set timestamp
  deviceData.lastUpdatedAt = String(millis());
  
  // Log to database using TinyGSMSupabase
  Serial.println("Sending data to Supabase...");
  int result = deviceDB->createDeviceData(deviceData);
  
  if (result == 200 || result == 201) {
    Serial.println("✓ Data logged successfully");
  } else {
    Serial.printf("✗ Failed to log data. HTTP Code: %d\n", result);
  }
  
  // Update device online status
  deviceDB->updateDeviceStatus(deviceId, true);
  
  // Print summary
  Serial.printf("CPU Temp: %.2f°C | Signal: %d%% | Battery: %.1f%% | TOF: %.1fmm\n",
                sensorData.cpuTemperature,
                sensorData.signalStrength,
                sensorData.batteryPercentage,
                sensorData.tof);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== Smart Echo Drain Device Starting ===");
  
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
  
  // Initialize sensors
  initializeSensors();
  
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
  
  // Log initial data
  SensorData initialData = readSensorData();
  logDeviceData(initialData);
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
  static unsigned long lastSensorRead = 0;
  
  const unsigned long DATA_LOG_INTERVAL = 60000;      // Log data every 60 seconds
  const unsigned long STATUS_PRINT_INTERVAL = 10000; // Print status every 10 seconds
  const unsigned long SENSOR_READ_INTERVAL = 5000;   // Read sensors every 5 seconds
  
  unsigned long currentTime = millis();
  
  // Read sensors periodically
  static SensorData currentSensorData;
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    currentSensorData = readSensorData();
    lastSensorRead = currentTime;
  }
  
  // Print status periodically
  if (currentTime - lastStatusPrint >= STATUS_PRINT_INTERVAL)
  {
    Serial.printf("[%lu] %s (%s) | %s | Temp: %.1f°C | Signal: %d%% | Next log: %lus\n", 
                  currentTime / 1000, 
                  deviceId.c_str(),
                  deviceSetup->getDeviceName().c_str(),
                  currentSensorData.device.status.c_str(),
                  currentSensorData.cpuTemperature,
                  currentSensorData.signalStrength,
                  (DATA_LOG_INTERVAL - (currentTime - lastDataLog)) / 1000);
    lastStatusPrint = currentTime;
  }
  
  // Log data periodically
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
    
    // Log current sensor data
    logDeviceData(currentSensorData);
    
    lastDataLog = currentTime;
  }
  
  // Handle critical conditions
  if (currentSensorData.cpuTemperature > 90) {
    Serial.println("⚠️ CRITICAL: CPU temperature too high! Entering safety mode...");
    delay(30000); // Reduce processing to cool down
  }
  
  // Small delay to prevent busy waiting
  delay(100);
}