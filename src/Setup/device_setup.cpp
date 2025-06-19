#include "device_setup.h"
#include <WiFi.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

DeviceSetup::DeviceSetup(const String& deviceId, Supabase* database) 
  : setupMode(false), setupCompleted(false), deviceId(deviceId), db(database) {
  server = new WebServer(80);
}

DeviceSetup::~DeviceSetup() {
  delete server;
}

bool DeviceSetup::isSetupCompleted() {
  EEPROM.begin(EEPROM_SIZE);
  uint16_t magic = (EEPROM.read(SETUP_FLAG_ADDR) << 8) | EEPROM.read(SETUP_FLAG_ADDR + 1);
  EEPROM.end();
  return magic == SETUP_MAGIC_NUMBER;
}

void DeviceSetup::markSetupCompleted() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(SETUP_FLAG_ADDR, (SETUP_MAGIC_NUMBER >> 8) & 0xFF);
  EEPROM.write(SETUP_FLAG_ADDR + 1, SETUP_MAGIC_NUMBER & 0xFF);
  EEPROM.commit();
  EEPROM.end();
  setupCompleted = true;
}

void DeviceSetup::resetSetupFlag() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(SETUP_FLAG_ADDR, 0);
  EEPROM.write(SETUP_FLAG_ADDR + 1, 0);
  EEPROM.commit();
  EEPROM.end();
}

void DeviceSetup::handleRoot() {
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
            <strong>Device ID:</strong> )" + deviceId + R"(<br>
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
  server->send(200, "text/html", html);
}

void DeviceSetup::handleSetup() {
  if (server->method() == HTTP_POST) {
    String body = server->arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);
    
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
    
    // TODO: Complete device setup via database
    // This would involve creating user, device, and address records
    
    markSetupCompleted();
    
    JsonDocument response;
    response["success"] = true;
    response["message"] = "Device setup completed successfully";
    response["device_id"] = deviceId;
    response["device_name"] = deviceName;
    response["full_location"] = deviceStreet + ", " + deviceLocation + " " + devicePostalCode;
    
    String responseStr;
    serializeJson(response, responseStr);
    server->send(200, "application/json", responseStr);
  } else {
    server->send(405, "text/plain", "Method Not Allowed");
  }
}

void DeviceSetup::handleRestart() {
  server->send(200, "text/html", 
    "<html><body><h1>Device Restarting...</h1><p>Setup completed. Device will now restart and begin normal operation.</p></body></html>");
  delay(2000);
  ESP.restart();
}

void DeviceSetup::startSetupMode() {
  Serial.println("\n=== Starting Setup Mode ===");
  setupMode = true;
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SETUP_SSID, SETUP_PASSWORD);
  
  Serial.printf("Setup WiFi started: %s\n", SETUP_SSID);
  Serial.printf("Password: %s\n", SETUP_PASSWORD);
  Serial.printf("IP Address: %s\n", WiFi.softAPIP().toString().c_str());
  
  server->on("/", [this]() { handleRoot(); });
  server->on("/setup", [this]() { handleSetup(); });
  server->on("/restart", [this]() { handleRestart(); });
  
  server->begin();
  Serial.println("Setup web server started");
  Serial.println("Connect to the WiFi network and visit http://192.168.4.1");
}

void DeviceSetup::loop() {
  if (setupMode && !setupCompleted) {
    server->handleClient();
    
    static unsigned long setupStartTime = millis();
    if (millis() - setupStartTime > SETUP_TIMEOUT) {
      Serial.println("Setup timeout reached. Restarting...");
      ESP.restart();
    }
  }
}

bool DeviceSetup::checkDeviceSetupStatus() {
  Serial.println("Checking device setup status...");
  
  String checkResult = db->rpc("check_device_setup", "{\"device_id\":\"" + deviceId + "\"}");
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