#define TINY_GSM_RX_BUFFER 1024
// #define DUMP_AT_COMMANDS

#include "utilities.h"
#include "configs.h"
#include <TinyGsmClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// Include TinyGSM Supabase library
#include <TinyGSMSupabase/Supabase.h>

// Setup mode configuration
#define SETUP_SSID "SmartEchoDrain"
#define SETUP_PASSWORD "echodrain25"
#define SETUP_TIMEOUT 300000  // 5 minutes setup timeout

// EEPROM addresses for storing setup completion flag
#define EEPROM_SIZE 512
#define SETUP_FLAG_ADDR 0
#define SETUP_MAGIC_NUMBER 0xABCD

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
Supabase db;
WebServer server(80);

// Global variables
bool setupMode = false;
bool setupCompleted = false;
String deviceName = "";
String deviceLocation = "";
String deviceStreet = "";
String devicePostalCode = "";

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

// Check if device setup is completed
bool isSetupCompleted() {
  EEPROM.begin(EEPROM_SIZE);
  uint16_t magic = (EEPROM.read(SETUP_FLAG_ADDR) << 8) | EEPROM.read(SETUP_FLAG_ADDR + 1);
  EEPROM.end();
  return magic == SETUP_MAGIC_NUMBER;
}

// Mark setup as completed
void markSetupCompleted() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(SETUP_FLAG_ADDR, (SETUP_MAGIC_NUMBER >> 8) & 0xFF);
  EEPROM.write(SETUP_FLAG_ADDR + 1, SETUP_MAGIC_NUMBER & 0xFF);
  EEPROM.commit();
  EEPROM.end();
  setupCompleted = true;
}

// Reset setup flag (for testing)
void resetSetupFlag() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(SETUP_FLAG_ADDR, 0);
  EEPROM.write(SETUP_FLAG_ADDR + 1, 0);
  EEPROM.commit();
  EEPROM.end();
}

// Web server handlers for device setup
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Device Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; margin-bottom: 30px; }
        .form-group { margin-bottom: 20px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }
        input, select { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; font-size: 16px; }
        button { width: 100%; padding: 12px; background: #007bff; color: white; border: none; border-radius: 5px; font-size: 16px; cursor: pointer; }
        button:hover { background: #0056b3; }
        .device-info { background: #e9ecef; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        .status { padding: 10px; border-radius: 5px; margin-top: 20px; text-align: center; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 Device Setup</h1>
        
        <div class="device-info">
            <strong>Device ID:</strong> )" + String(DEVICE_ID) + R"(<br>
            <strong>Firmware:</strong> )" + String(DEVICE_VERSION) + R"(<br>
            <strong>Chip Model:</strong> )" + ESP.getChipModel() + R"(
        </div>

        <form id="setupForm">
            <div class="form-group">
                <label for="deviceName">Device Name:</label>
                <input type="text" id="deviceName" name="deviceName" required placeholder="e.g., Sensor Node 001">
            </div>
            
            <div class="form-group">
                <label for="location">Location:</label>
                <input type="text" id="location" name="location" required placeholder="e.g., Manila, Philippines">
            </div>
            
            <div class="form-group">
                <label for="street">Street Address:</label>
                <input type="text" id="street" name="street" required placeholder="e.g., 123 Sample Street, Barangay ABC">
            </div>
            
            <div class="form-group">
                <label for="postalCode">Postal Code:</label>
                <input type="text" id="postalCode" name="postalCode" required placeholder="e.g., 1000">
            </div>
            
            <div class="form-group">
                <label for="email">Admin Email:</label>
                <input type="email" id="email" name="email" required placeholder="admin@example.com">
            </div>
            
            <div class="form-group">
                <label for="password">Admin Password:</label>
                <input type="password" id="password" name="password" required placeholder="Minimum 8 characters">
            </div>
            
            <button type="submit">Complete Setup</button>
        </form>
        
        <div id="status" class="status" style="display: none;"></div>
    </div>

    <script>
        document.getElementById('setupForm').addEventListener('submit', async function(e) {
            e.preventDefault();
            
            const formData = new FormData(e.target);
            const data = Object.fromEntries(formData.entries());
            
            document.getElementById('status').style.display = 'none';
            
            try {
                const response = await fetch('/setup', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(data)
                });
                
                const result = await response.json();
                const statusDiv = document.getElementById('status');
                statusDiv.style.display = 'block';
                
                if (response.ok && result.success) {
                    statusDiv.className = 'status success';
                    statusDiv.innerHTML = '✅ Setup completed successfully! Device will restart and begin data logging.';
                    setTimeout(() => {
                        window.location.href = '/restart';
                    }, 3000);
                } else {
                    statusDiv.className = 'status error';
                    statusDiv.innerHTML = '❌ Setup failed: ' + (result.message || 'Unknown error');
                }
            } catch (error) {
                const statusDiv = document.getElementById('status');
                statusDiv.style.display = 'block';
                statusDiv.className = 'status error';
                statusDiv.innerHTML = '❌ Network error: ' + error.message;
            }
        });
    </script>
</body>
</html>
)";
  server.send(200, "text/html", html);
}

void handleSetup() {
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);
    
    // Extract setup data
    deviceName = doc["deviceName"].as<String>();
    deviceLocation = doc["location"].as<String>();
    deviceStreet = doc["street"].as<String>();
    devicePostalCode = doc["postalCode"].as<String>();
    String email = doc["email"].as<String>();
    String password = doc["password"].as<String>();
    
    Serial.println("Setup data received:");
    Serial.println("Device Name: " + deviceName);
    Serial.println("Location: " + deviceLocation);
    Serial.println("Street: " + deviceStreet);
    Serial.println("Postal Code: " + devicePostalCode);
    
    // For now, we'll simulate successful setup
    // In a real implementation, you would:
    // 1. Connect to internet via cellular
    // 2. Create user account in Supabase
    // 3. Complete device setup via RPC calls
    
    markSetupCompleted();
    
    JsonDocument response;
    response["success"] = true;
    response["message"] = "Device setup completed successfully";
    response["device_id"] = DEVICE_ID;
    response["device_name"] = deviceName;
    response["full_location"] = deviceStreet + ", " + deviceLocation + " " + devicePostalCode;
    
    String responseStr;
    serializeJson(response, responseStr);
    server.send(200, "application/json", responseStr);
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void handleRestart() {
  server.send(200, "text/html", 
    "<html><body><h1>Device Restarting...</h1><p>Setup completed. Device will now restart and begin normal operation.</p></body></html>");
  delay(2000);
  ESP.restart();
}

void startSetupMode() {
  Serial.println("\n=== Starting Setup Mode ===");
  setupMode = true;
  
  // Start WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SETUP_SSID, SETUP_PASSWORD);
  
  Serial.printf("Setup WiFi started: %s\n", SETUP_SSID);
  Serial.printf("Password: %s\n", SETUP_PASSWORD);
  Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/setup", handleSetup);
  server.on("/restart", handleRestart);
  
  server.begin();
  Serial.println("Setup web server started");
  Serial.println("Connect to the WiFi network and visit http://192.168.4.1");
}

void handleSetupMode() {
  server.handleClient();
  
  // Check for setup timeout
  static unsigned long setupStartTime = millis();
  if (millis() - setupStartTime > SETUP_TIMEOUT) {
    Serial.println("Setup timeout reached. Restarting...");
    ESP.restart();
  }
}

void initializeModem()
{
  Serial.begin(115200);
  Serial.println("=== TinyGSM Supabase Data Logger ===");
  Serial.printf("Device ID: %s\n", DEVICE_ID);
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

  // Set APN
  Serial.printf("Setting APN: %s\n", NETWORK_APN);
  modem.sendAT(GF("+CGDCONT=1,\"IP\",\""), NETWORK_APN, "\"");
  if (modem.waitResponse() != 1)
  {
    Serial.println("Set APN error!");
  }

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

  // Connect to GPRS
  Serial.print("Connecting to GPRS...");
  if (!modem.gprsConnect(NETWORK_APN))
  {
    Serial.println("GPRS connection failed!");
    return;
  }
  Serial.println("GPRS connected");

  String ipAddress = modem.getLocalIP();
  Serial.print("Local IP: ");
  Serial.println(ipAddress);

  Serial.println("Modem initialization complete!");
}

bool checkDeviceSetupStatus() {
  Serial.println("Checking device setup status...");
  
  // Check via Supabase if device is properly setup
  String checkResult = db.rpc("check_device_setup", "{\"device_id\":\"" + String(DEVICE_ID) + "\"}");
  Serial.println("Setup check result: " + checkResult);
  
  JsonDocument doc;
  deserializeJson(doc, checkResult);
  
  if (doc["setup_completed"].as<bool>() == true) {
    deviceName = doc["device_name"].as<String>();
    deviceLocation = doc["location"].as<String>();
    Serial.println("✓ Device setup completed previously");
    Serial.printf("Device Name: %s\n", deviceName.c_str());
    Serial.printf("Location: %s\n", deviceLocation.c_str());
    return true;
  }
  
  Serial.println("✗ Device setup required");
  return false;
}

void initializeSensors()
{
  Serial.println("Initializing sensors...");
  
  // Initialize ultrasonic sensors
  pinMode(ULTRASONIC1_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC1_ECHO_PIN, INPUT);
  pinMode(ULTRASONIC2_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC2_ECHO_PIN, INPUT);
  
  Serial.println("Sensors initialized!");
}

float readUltrasonicDistance(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2; // Convert to cm
  
  return distance;
}

SensorData readAllSensors()
{
  SensorData data;
  
  // Read device sensors
  data.cpuTemperature = temperatureRead(); // ESP32 internal temperature
  data.signalStrength = modem.getSignalQuality();
  data.batteryVoltage = 3.7; // Read actual battery voltage if available
  data.firmwareVersion = DEVICE_VERSION;
  data.uptime = millis();
  
  // Read module sensors
  data.tof1 = analogRead(TOF1_PIN) * 0.1; // Convert to appropriate units
  data.tof2 = analogRead(TOF2_PIN) * 0.1;
  data.force1 = analogRead(FORCE1_PIN) * 0.1;
  data.force2 = analogRead(FORCE2_PIN) * 0.1;
  data.weight1 = analogRead(WEIGHT1_PIN) * 0.1;
  data.weight2 = analogRead(WEIGHT2_PIN) * 0.1;
  data.turbidity1 = analogRead(TURBIDITY1_PIN) * 0.1;
  data.turbidity2 = analogRead(TURBIDITY2_PIN) * 0.1;
  data.ultrasonic1 = readUltrasonicDistance(ULTRASONIC1_TRIG_PIN, ULTRASONIC1_ECHO_PIN);
  data.ultrasonic2 = readUltrasonicDistance(ULTRASONIC2_TRIG_PIN, ULTRASONIC2_ECHO_PIN);
  
  // Determine DEVICE status based on device-specific factors
  if (data.batteryVoltage < 3.6)
  {
    data.device.status = "error";
    data.device.message = "Critical battery level";
  }
  else if (data.batteryVoltage < 3.7)
  {
    data.device.status = "warning";
    data.device.message = "Low battery warning";
  }
  else if (data.signalStrength < 10)
  {
    data.device.status = "warning";
    data.device.message = "Low signal strength detected";
  }
  else if (data.cpuTemperature > 85.0) // ESP32 critical temperature
  {
    data.device.status = "warning";
    data.device.message = "High CPU temperature";
  }
  else
  {
    data.device.status = "online";
    data.device.message = "Device operational";
  }
  
  // Determine MODULE status based on sensor readings
  bool sensorError = false;
  bool sensorWarning = false;
  String moduleIssues = "";
  
  // Check for sensor errors (invalid readings)
  if (isnan(data.tof1) || isnan(data.tof2))
  {
    sensorError = true;
    moduleIssues += "ToF sensor failure; ";
  }
  
  if (data.ultrasonic1 > 500 || data.ultrasonic2 > 500 || data.ultrasonic1 < 2 || data.ultrasonic2 < 2)
  {
    sensorWarning = true;
    moduleIssues += "Ultrasonic out of range; ";
  }
  
  if (data.force1 < 0 || data.force2 < 0)
  {
    sensorWarning = true;
    moduleIssues += "Force sensor anomaly; ";
  }
  
  if (data.turbidity1 > 3000 || data.turbidity2 > 3000) // High turbidity
  {
    sensorWarning = true;
    moduleIssues += "High turbidity detected; ";
  }
  
  // Set module status
  if (sensorError)
  {
    data.module.status = "error";
    data.module.message = "Sensor failure: " + moduleIssues;
  }
  else if (sensorWarning)
  {
    data.module.status = "warning";
    data.module.message = "Sensor issues: " + moduleIssues;
  }
  else
  {
    data.module.status = "online";
    data.module.message = "All modules operational";
  }
  
  return data;
}

String createSupabaseJSON(const SensorData& sensorData)
{
  JsonDocument doc;
  
  // Create device object with status
  JsonObject device = doc["device"].to<JsonObject>();
  device["id"] = DEVICE_ID;
  device["name"] = deviceName;
  device["cpu_temperature"] = sensorData.cpuTemperature;
  device["signal_strength"] = sensorData.signalStrength;
  device["battery_voltage"] = sensorData.batteryVoltage;
  device["firmware_version"] = sensorData.firmwareVersion;
  device["uptime_ms"] = sensorData.uptime;
  device["free_heap"] = ESP.getFreeHeap();
  device["chip_model"] = ESP.getChipModel();
  device["status"] = sensorData.device.status;
  device["message"] = sensorData.device.message;
  
  // Create module object with status
  JsonObject module = doc["module"].to<JsonObject>();
  module["tof1"] = sensorData.tof1;
  module["tof2"] = sensorData.tof2;
  module["force1"] = sensorData.force1;
  module["force2"] = sensorData.force2;
  module["weight1"] = sensorData.weight1;
  module["weight2"] = sensorData.weight2;
  module["turbidity1"] = sensorData.turbidity1;
  module["turbidity2"] = sensorData.turbidity2;
  module["ultrasonic1"] = sensorData.ultrasonic1;
  module["ultrasonic2"] = sensorData.ultrasonic2;
  module["status"] = sensorData.module.status;
  module["message"] = sensorData.module.message;
  
  // Add overall status based on worst case between device and module
  String overallStatus = "online";
  String overallMessage = "All systems operational";
  
  if (sensorData.device.status == "error" || sensorData.module.status == "error")
  {
    overallStatus = "error";
    overallMessage = "System errors detected";
  }
  else if (sensorData.device.status == "warning" || sensorData.module.status == "warning")
  {
    overallStatus = "warning";
    overallMessage = "System warnings detected";
  }
  
  doc["status"] = overallStatus;
  doc["message"] = overallMessage;
  
  // Add timestamp (ISO 8601 format)
  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ", 
           2025, 6, 8, 12, 0, 0); // You would get actual time from NTP or RTC
  doc["timestamp"] = timestamp;
  
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

bool logDataToSupabase(const SensorData& sensorData)
{
  Serial.println("\n=== Logging Data to Supabase ===");
  
  // Create JSON payload
  String jsonPayload = createSupabaseJSON(sensorData);
  Serial.println("JSON Payload:");
  Serial.println(jsonPayload);
  
  // Insert data into Supabase (trigger will automatically update device status)
  int result = db.insert("data", jsonPayload, false);
  
  Serial.print("Insert result: ");
  Serial.println(result);
  
  if (result >= 200 && result < 300)
  {
    Serial.println("✓ Data logged successfully!");
    return true;
  }
  else
  {
    Serial.println("✗ Failed to log data!");
    return false;
  }
}

void printSensorData(const SensorData& data)
{
  Serial.println("\n=== Sensor Readings ===");
  Serial.printf("Device ID: %s\n", DEVICE_ID);
  Serial.printf("Device Name: %s\n", deviceName.c_str());
  Serial.printf("Location: %s\n", deviceLocation.c_str());
  Serial.printf("CPU Temperature: %.2f°C\n", data.cpuTemperature);
  Serial.printf("Signal Strength: %d\n", data.signalStrength);
  Serial.printf("Battery Voltage: %.2fV\n", data.batteryVoltage);
  Serial.printf("Uptime: %lu ms\n", data.uptime);
  
  Serial.println("\n=== Module Readings ===");
  Serial.printf("ToF1: %.2f\n", data.tof1);
  Serial.printf("ToF2: %.2f\n", data.tof2);
  Serial.printf("Force1: %.2f\n", data.force1);
  Serial.printf("Force2: %.2f\n", data.force2);
  Serial.printf("Weight1: %.2f\n", data.weight1);
  Serial.printf("Weight2: %.2f\n", data.weight2);
  Serial.printf("Turbidity1: %.2f\n", data.turbidity1);
  Serial.printf("Turbidity2: %.2f\n", data.turbidity2);
  Serial.printf("Ultrasonic1: %.2f cm\n", data.ultrasonic1);
  Serial.printf("Ultrasonic2: %.2f cm\n", data.ultrasonic2);
  
  Serial.println("\n=== Status Summary ===");
  Serial.printf("Device Status: %s - %s\n", data.device.status.c_str(), data.device.message.c_str());
  Serial.printf("Module Status: %s - %s\n", data.module.status.c_str(), data.module.message.c_str());
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== Device Starting ===");
  
  // Check if setup is completed locally
  setupCompleted = isSetupCompleted();
  
  if (!setupCompleted) {
    Serial.println("Setup required - Starting setup mode...");
    startSetupMode();
    return;
  }
  
  Serial.println("Setup completed - Starting normal operation...");
  
  // Initialize modem and network
  initializeModem();
  
  // Initialize Supabase
  Serial.println("Initializing Supabase connection...");
  db.begin(SUPABASE_URL, SUPABASE_ANON_KEY, modem, client);
  
  // Check setup status in database
  if (!checkDeviceSetupStatus()) {
    Serial.println("Database setup incomplete - requires online setup");
    // Could enter setup mode again or continue with default values
  }
  
  // Initialize sensors
  initializeSensors();
  
  // Send initial data to mark device as online
  SensorData initialData = readAllSensors();
  logDataToSupabase(initialData);
  
  Serial.println("=== Setup Complete ===");
  Serial.println("Starting data logging loop...\n");
  Serial.printf("Device %s (%s) is now operational\n", DEVICE_ID, deviceName.c_str());
}

void loop()
{
  // Handle setup mode
  if (setupMode && !setupCompleted) {
    handleSetupMode();
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
                  DEVICE_ID,
                  deviceName.c_str(),
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
    
    // Read all sensors
    SensorData sensorData = readAllSensors();
    
    // Print sensor data to console
    printSensorData(sensorData);
    
    // Log to Supabase (this will automatically update device status via trigger)
    bool success = logDataToSupabase(sensorData);
    
    if (success)
    {
      Serial.printf("✓ %s data logging cycle completed successfully!\n", deviceName.c_str());
    }
    else
    {
      Serial.printf("✗ %s data logging failed. Will retry next cycle.\n", deviceName.c_str());
    }
    
    lastDataLog = currentTime;
  }
  
  // Small delay to prevent busy waiting
  delay(100);
}