#include "configs.h"
#include "Utils/utilities.h"
#include "Utils/device_id.h"
#include "Setup/device_setup.h"
#include "Database/device_db.h"
#include "Database/address.h"
#include <HX711.h>
#include <Adafruit_VL53L0X.h>
#include <WiFi.h>
#include <esp32-hal-adc.h>

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

// Sensor objects
HX711 weightSensor;
Adafruit_VL53L0X tofSensor = Adafruit_VL53L0X();

// Function declarations
bool initializeModem();
bool initializeSensors();
DeviceData collectSensorData();
float readForce(int pin);
float readTurbidity();
float readUltrasonic();
float getBatteryVoltage();
float getBatteryPercentage(float voltage);
float getSolarVoltage();
float getCPUTemperature();
float getCPUFrequency();
float getRAMUsage();
float getStorageUsage();
float getSignalStrength();
unsigned long getUptime();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Smart Echo Drain ESP32 Starting ===");
  Serial.printf("Device Version: %s\n", DEVICE_VERSION);
  Serial.printf("API Key: %s\n", ESP32_API_KEY);
  
  // Initialize ADC for battery monitoring
  analogSetAttenuation(ADC_11db);
  analogReadResolution(12);
#if CONFIG_IDF_TARGET_ESP32
  analogSetWidth(12);
#endif

#ifdef BOARD_POWERON_PIN
  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif
  
  // Generate or get device ID
  deviceId = getOrGenerateDeviceId();
  Serial.printf("Device ID: %s\n", deviceId.c_str());

  // Initialize sensors
  if (!initializeSensors()) {
    Serial.println("Warning: Some sensors failed to initialize");
  }

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
      
      // Check if device exists in database
      if (deviceSetup->checkDeviceSetupStatus()) {
        Serial.println("✓ Device verified in database");
        Serial.printf("Device Name: %s\n", deviceSetup->getDeviceName().c_str());
        Serial.printf("Location: %s\n", deviceSetup->getDeviceLocation().c_str());
        
        // Send initial device data
        DeviceData initialData = collectSensorData();
        int result = deviceDB->createDeviceData(initialData);
        Serial.printf("Initial device data sent - Status: %d\n", result);
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
    static unsigned long lastDataSend = 0;
    
    // Send sensor data every 30 seconds (adjustable based on your needs)
    if (millis() - lastDataSend > 30000) {
      if (deviceDB) {
        // Collect real sensor data
        DeviceData sensorData = collectSensorData();
        
        // Send data to database
        int result = deviceDB->createDeviceData(sensorData);
        Serial.printf("Sensor data sent - Status: %d\n", result);
        
        // Print sensor readings for debugging
        Serial.println("\n=== Sensor Readings ===");
        Serial.printf("Battery: %.2fV (%.1f%%)\n", sensorData.batteryVoltage, sensorData.batteryPercentage);
        Serial.printf("Solar: %.2fW\n", sensorData.solarWattage);
        Serial.printf("TOF: %.2f mm\n", sensorData.tof);
        Serial.printf("Force0: %.2f N\n", sensorData.force0);
        Serial.printf("Force1: %.2f N\n", sensorData.force1);
        Serial.printf("Weight: %.2f kg\n", sensorData.weight);
        Serial.printf("Turbidity: %.2f NTU\n", sensorData.turbidity);
        Serial.printf("Ultrasonic: %.2f cm\n", sensorData.ultrasonic);
        Serial.printf("CPU Temp: %.1f°C\n", sensorData.cpuTemperature);
        Serial.printf("RAM Usage: %.1f%%\n", sensorData.ramUsage);
        Serial.printf("Signal: %.1f dBm\n", sensorData.signalStrength);
        Serial.printf("Uptime: %lu ms\n", sensorData.uptimeMs);
        Serial.println("========================\n");
      }
      lastDataSend = millis();
    }
    
    delay(1000); // Small delay to prevent overwhelming the system
  }
}

bool initializeSensors() {
  Serial.println("\n=== Initializing Sensors ===");
  bool success = true;
  
  // Initialize I2C for TOF sensor
  Wire.begin(TOF_SDA_PIN, TOF_SCL_PIN);
  
  // Initialize TOF sensor (Adafruit VL53L0X)
  if (tofSensor.begin()) {
    Serial.println("TOF sensor initialized successfully");
    
    // Optional: Configure sensor settings
    tofSensor.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);
    
    // Set timing budget (optional)
    tofSensor.setMeasurementTimingBudgetMicroSeconds(30000); // 30ms
    
    // Start continuous ranging
    tofSensor.startRangeContinuous();
  } else {
    Serial.println("Failed to initialize TOF sensor");
    success = false;
  }
  
  // Initialize weight sensor (HX711)
  weightSensor.begin(WEIGHT_DATA_PIN, WEIGHT_SCK_PIN);
  if (weightSensor.is_ready()) {
    weightSensor.set_scale(2280.f); // Calibration factor - adjust based on your setup
    weightSensor.tare(); // Reset the scale to 0
    Serial.println("Weight sensor initialized successfully");
  } else {
    Serial.println("Failed to initialize weight sensor");
    success = false;
  }
  
  // Initialize analog pins for force sensors
  pinMode(FORCE0_ANALOG_PIN, INPUT);
  pinMode(FORCE1_ANALOG_PIN, INPUT);
  pinMode(TURBIDITY_ANALOG_PIN, INPUT);
  
  // Initialize ultrasonic sensor pins
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  
  Serial.println("Analog sensors initialized");
  Serial.println("========================\n");
  
  return success;
}

DeviceData collectSensorData() {
  DeviceData data;
  
  // Basic device info
  data.deviceId = deviceId;
  data.isOnline = true;
  data.createdAt = String(millis());
  data.lastUpdatedAt = String(millis());
  
  // System monitoring
  data.cpuTemperature = getCPUTemperature();
  data.cpuFrequency = getCPUFrequency();
  data.ramUsage = getRAMUsage();
  data.storageUsage = getStorageUsage();
  data.signalStrength = getSignalStrength();
  data.uptimeMs = getUptime();
  
  // Battery monitoring
  data.batteryVoltage = getBatteryVoltage();
  data.batteryPercentage = getBatteryPercentage(data.batteryVoltage);
  data.solarWattage = getSolarVoltage();
  
  // Set battery status based on voltage
  if (data.batteryVoltage > 4.0) {
    data.batteryStatus = "good";
  } else if (data.batteryVoltage > 3.7) {
    data.batteryStatus = "medium";
  } else if (data.batteryVoltage > 3.3) {
    data.batteryStatus = "low";
  } else {
    data.batteryStatus = "critical";
  }
  
  // TOF sensor reading (Adafruit VL53L0X)
  VL53L0X_RangingMeasurementData_t measure;
  tofSensor.rangingTest(&measure, false); // pass in 'true' to get debug data printed
  
  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    data.tof = measure.RangeMilliMeter;
  } else {
    data.tof = -1; // Sensor error/timeout
  }
  
  // Other sensor readings
  data.force0 = readForce(FORCE0_ANALOG_PIN);
  data.force1 = readForce(FORCE1_ANALOG_PIN);
  
  if (weightSensor.is_ready()) {
    data.weight = weightSensor.get_units(10); // Average of 10 readings
  } else {
    data.weight = 0.0;
  }
  
  data.turbidity = readTurbidity();
  data.ultrasonic = readUltrasonic();
  
  // Device status information
  data.deviceStatus["modem"] = modem.isNetworkConnected() ? "connected" : "disconnected";
  data.deviceStatus["sensors"] = "active";
  data.deviceStatus["power"] = data.batteryVoltage > 3.3 ? "normal" : "low";
  
  // Module status information
  data.moduleStatus["tof"] = (measure.RangeStatus != 4) ? "online" : "error";
  data.moduleStatus["weight"] = weightSensor.is_ready() ? "online" : "offline";
  data.moduleStatus["force"] = "online";
  data.moduleStatus["turbidity"] = "online";
  data.moduleStatus["ultrasonic"] = "online";
  
  // Additional device data
  data.deviceOtherData["free_heap"] = String(ESP.getFreeHeap());
  data.deviceOtherData["chip_revision"] = String(ESP.getChipRevision());
  data.deviceOtherData["sdk_version"] = ESP.getSdkVersion();
  
  // Additional module data
  data.moduleOtherData["force0_raw"] = String(analogRead(FORCE0_ANALOG_PIN));
  data.moduleOtherData["force1_raw"] = String(analogRead(FORCE1_ANALOG_PIN));
  data.moduleOtherData["turbidity_raw"] = String(analogRead(TURBIDITY_ANALOG_PIN));
  data.moduleOtherData["weight_raw"] = String(weightSensor.get_value());
  data.moduleOtherData["tof_status"] = String(measure.RangeStatus);
  
  return data;
}

float readForce(int pin) {
  int rawValue = analogRead(pin);
  // Convert to force in Newtons (adjust calibration based on your sensor)
  // Assuming FSR sensor with voltage divider
  float voltage = (rawValue / 4095.0) * 3.3;
  float resistance = 10000.0 * (3.3 - voltage) / voltage; // 10k pull-down resistor
  float force = 1.0 / (resistance / 1000.0); // Convert to approximate force
  return force;
}

float readTurbidity() {
  int rawValue = analogRead(TURBIDITY_ANALOG_PIN);
  float voltage = (rawValue / 4095.0) * 3.3;
  // Convert voltage to NTU (adjust calibration based on your sensor)
  float turbidity = (voltage - 2.5) * 3000.0 / 2.0; // Example conversion
  return max(0.0f, turbidity);
}

float readUltrasonic() {
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  
  long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) {
    return -1; // Timeout or error
  }
  
  float distance = duration * 0.034 / 2; // Convert to cm
  return distance;
}

float getBatteryVoltage() {
#ifdef BOARD_BAT_ADC_PIN
  uint32_t battery_voltage = analogReadMilliVolts(BOARD_BAT_ADC_PIN);
  battery_voltage *= 2; // Hardware voltage divider
  return battery_voltage / 1000.0; // Convert to volts
#else
  return 3.7; // Default value if no battery monitoring
#endif
}

float getBatteryPercentage(float voltage) {
  // Li-ion battery voltage to percentage conversion
  if (voltage >= 4.2) return 100.0;
  if (voltage >= 4.0) return 75.0 + ((voltage - 4.0) / 0.2) * 25.0;
  if (voltage >= 3.8) return 50.0 + ((voltage - 3.8) / 0.2) * 25.0;
  if (voltage >= 3.6) return 25.0 + ((voltage - 3.6) / 0.2) * 25.0;
  if (voltage >= 3.3) return 0.0 + ((voltage - 3.3) / 0.3) * 25.0;
  return 0.0;
}

float getSolarVoltage() {
#ifdef BOARD_SOLAR_ADC_PIN
  uint32_t solar_voltage = analogReadMilliVolts(BOARD_SOLAR_ADC_PIN);
  solar_voltage *= 2; // Hardware voltage divider
  float voltage = solar_voltage / 1000.0;
  // Approximate wattage calculation (adjust based on your solar panel specs)
  float current = voltage / 10.0; // Assuming 10 ohm load for approximation
  return voltage * current; // Power in watts
#else
  return 0.0; // No solar monitoring
#endif
}

float getCPUTemperature() {
  // ESP32 internal temperature sensor
  return temperatureRead();
}

float getCPUFrequency() {
  // Get CPU frequency in MHz
  return getCpuFrequencyMhz();
}

float getRAMUsage() {
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t totalHeap = ESP.getHeapSize();
  return ((totalHeap - freeHeap) * 100.0) / totalHeap;
}

float getStorageUsage() {
  // For flash storage usage (simplified)
  size_t totalBytes = ESP.getFlashChipSize();
  size_t sketchSize = ESP.getSketchSize();
  return (sketchSize * 100.0) / totalBytes;
}

float getSignalStrength() {
  if (modem.isNetworkConnected()) {
    return modem.getSignalQuality();
  }
  return -999; // No signal
}

unsigned long getUptime() {
  return millis();
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