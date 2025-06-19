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

void DeviceSetup::handleAddressData() {
  if (server->method() == HTTP_GET) {
    String regCode = server->arg("reg_code");
    String provCode = server->arg("prov_code");
    String cityMunCode = server->arg("citymun_code");
    
    // Create AddressDB instance temporarily for RPC call
    AddressDB* addressDB = new AddressDB(db);
    String result = addressDB->getAddressDropdownData(regCode, provCode, cityMunCode);
    delete addressDB;
    
    server->send(200, "application/json", result);
  } else {
    server->send(405, "text/plain", "Method Not Allowed");
  }
}

void DeviceSetup::handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Echo Drain - Device Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 15px; box-shadow: 0 10px 30px rgba(0,0,0,0.3); }
        h1 { color: #333; text-align: center; margin-bottom: 10px; font-size: 2em; }
        .subtitle { text-align: center; color: #666; margin-bottom: 30px; }
        .form-group { margin-bottom: 20px; }
        .form-row { display: flex; gap: 15px; }
        .form-row .form-group { flex: 1; }
        label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }
        input, select { width: 100%; padding: 12px; border: 2px solid #ddd; border-radius: 8px; font-size: 16px; transition: border-color 0.3s; box-sizing: border-box; }
        input:focus, select:focus { outline: none; border-color: #667eea; }
        button { width: 100%; padding: 15px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; border-radius: 8px; font-size: 18px; font-weight: bold; cursor: pointer; transition: transform 0.2s; }
        button:hover { transform: translateY(-2px); }
        button:disabled { opacity: 0.6; cursor: not-allowed; transform: none; }
        .device-info { background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); color: white; padding: 20px; border-radius: 10px; margin-bottom: 30px; }
        .device-info h3 { margin: 0 0 15px 0; }
        .status { padding: 15px; border-radius: 8px; margin-top: 20px; text-align: center; font-weight: bold; }
        .success { background: #d4edda; color: #155724; border: 2px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 2px solid #f5c6cb; }
        .loading { background: #fff3cd; color: #856404; border: 2px solid #ffeaa7; }
        .address-section { background: #f8f9fa; padding: 20px; border-radius: 10px; margin: 20px 0; }
        .progress-bar { width: 100%; height: 4px; background: #eee; border-radius: 2px; margin-bottom: 20px; overflow: hidden; }
        .progress-fill { height: 100%; background: linear-gradient(90deg, #667eea, #764ba2); transition: width 0.5s ease; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌊 Smart Echo Drain</h1>
        <p class="subtitle">Device Configuration & Setup</p>
        
        <div class="progress-bar">
            <div class="progress-fill" id="progressFill" style="width: 0%"></div>
        </div>
        
        <div class="device-info">
            <h3>📱 Device Information</h3>
            <strong>Device ID:</strong> )" + deviceId + R"(<br>
            <strong>Firmware:</strong> )" + String(DEVICE_VERSION) + R"(<br>
            <strong>Chip:</strong> )" + ESP.getChipModel() + R"(<br>
            <strong>Temperature:</strong> )" + temperatureRead() + R"(&#8451;<br>
        </div>

        <form id="setupForm">
            <div class="form-group">
                <label for="deviceName">🏷️ Device Name:</label>
                <input type="text" id="deviceName" name="deviceName" required 
                       placeholder="e.g., Echo Drain Sensor 001" maxlength="50">
            </div>
            
            <div class="address-section">
                <h3>📍 Device Location</h3>
                
                <div class="form-row">
                    <div class="form-group">
                        <label for="country">Country:</label>
                        <input type="text" id="country" name="country" value="PHILIPPINES" readonly>
                    </div>
                    <div class="form-group">
                        <label for="region">Region:</label>
                        <select id="region" name="region" required>
                            <option value="">Select Region...</option>
                        </select>
                    </div>
                </div>
                
                <div class="form-row">
                    <div class="form-group">
                        <label for="province">Province:</label>
                        <select id="province" name="province" required disabled>
                            <option value="">Select Province...</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="municipality">City/Municipality:</label>
                        <select id="municipality" name="municipality" required disabled>
                            <option value="">Select City/Municipality...</option>
                        </select>
                    </div>
                </div>
                
                <div class="form-row">
                    <div class="form-group">
                        <label for="barangay">Barangay:</label>
                        <select id="barangay" name="barangay" required disabled>
                            <option value="">Select Barangay...</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="postalCode">Postal Code:</label>
                        <input type="text" id="postalCode" name="postalCode" required 
                               placeholder="e.g., 1000" pattern="[0-9]{4,5}" maxlength="5">
                    </div>
                </div>
                
                <div class="form-group">
                    <label for="street">Street Address:</label>
                    <input type="text" id="street" name="street" required 
                           placeholder="e.g., 123 Sample Street, Subdivision ABC" maxlength="100">
                </div>
                
                <div class="form-group">
                    <label for="fullAddress">Full Address (Auto-generated):</label>
                    <input type="text" id="fullAddress" name="fullAddress" readonly 
                           placeholder="Complete address will appear here...">
                </div>
            </div>
            
            <div class="address-section">
                <h3>👤 Administrator Account</h3>
                
                <div class="form-group">
                    <label for="email">📧 Admin Email:</label>
                    <input type="email" id="email" name="email" required 
                           placeholder="admin@example.com" maxlength="100">
                </div>
                
                <div class="form-group">
                    <label for="password">🔒 Admin Password:</label>
                    <input type="password" id="password" name="password" required 
                           placeholder="Minimum 8 characters" minlength="8">
                </div>
            </div>
            
            <button type="submit" id="submitBtn">🚀 Complete Setup</button>
        </form>
        
        <div id="status" class="status" style="display: none;"></div>
    </div>

    <script>
        let addressData = { regions: [], provinces: [], cities: [], barangays: [] };
        let selectedCodes = { region: '', province: '', municipality: '', barangay: '' };
        
        // Initialize form
        document.addEventListener('DOMContentLoaded', function() {
            loadRegions();
            updateProgress();
            
            // Add event listeners for cascading dropdowns
            document.getElementById('region').addEventListener('change', handleRegionChange);
            document.getElementById('province').addEventListener('change', handleProvinceChange);
            document.getElementById('municipality').addEventListener('change', handleMunicipalityChange);
            document.getElementById('barangay').addEventListener('change', handleBarangayChange);
            document.getElementById('street').addEventListener('input', updateFullAddress);
            document.getElementById('postalCode').addEventListener('input', updateFullAddress);
        });
        
        async function loadAddressData(regCode = '', provCode = '', cityMunCode = '') {
            try {
                const params = new URLSearchParams();
                if (regCode) params.append('reg_code', regCode);
                if (provCode) params.append('prov_code', provCode);
                if (cityMunCode) params.append('citymun_code', cityMunCode);
                
                const response = await fetch('/address_data?' + params.toString());
                const data = await response.json();
                
                addressData = {
                    regions: data.regions || [],
                    provinces: data.provinces || [],
                    cities: data.cities || [],
                    barangays: data.barangays || []
                };
                
                return data;
            } catch (error) {
                console.error('Error loading address data:', error);
                return { regions: [], provinces: [], cities: [], barangays: [] };
            }
        }
        
        async function loadRegions() {
            const data = await loadAddressData();
            populateSelect('region', data.regions, 'reg_code', 'reg_desc');
        }
        
        async function handleRegionChange() {
            const regionSelect = document.getElementById('region');
            const regCode = regionSelect.value;
            selectedCodes.region = regCode;
            
            if (regCode) {
                const data = await loadAddressData(regCode);
                populateSelect('province', data.provinces, 'prov_code', 'prov_desc');
                document.getElementById('province').disabled = false;
                
                // Reset subsequent dropdowns
                resetSelect('municipality');
                resetSelect('barangay');
            } else {
                resetSelect('province');
                resetSelect('municipality');
                resetSelect('barangay');
            }
            updateFullAddress();
            updateProgress();
        }
        
        async function handleProvinceChange() {
            const provinceSelect = document.getElementById('province');
            const provCode = provinceSelect.value;
            selectedCodes.province = provCode;
            
            if (provCode) {
                const data = await loadAddressData('', provCode);
                populateSelect('municipality', data.cities, 'citymun_code', 'citymun_desc');
                document.getElementById('municipality').disabled = false;
                
                // Reset subsequent dropdown
                resetSelect('barangay');
            } else {
                resetSelect('municipality');
                resetSelect('barangay');
            }
            updateFullAddress();
            updateProgress();
        }
        
        async function handleMunicipalityChange() {
            const municipalitySelect = document.getElementById('municipality');
            const cityMunCode = municipalitySelect.value;
            selectedCodes.municipality = cityMunCode;
            
            if (cityMunCode) {
                const data = await loadAddressData('', '', cityMunCode);
                populateSelect('barangay', data.barangays, 'brgy_code', 'brgy_desc');
                document.getElementById('barangay').disabled = false;
            } else {
                resetSelect('barangay');
            }
            updateFullAddress();
            updateProgress();
        }
        
        function handleBarangayChange() {
            selectedCodes.barangay = document.getElementById('barangay').value;
            updateFullAddress();
            updateProgress();
        }
        
        function populateSelect(selectId, data, codeField, descField) {
            const select = document.getElementById(selectId);
            select.innerHTML = '<option value="">Select ' + selectId.charAt(0).toUpperCase() + selectId.slice(1) + '...</option>';
            
            data.forEach(item => {
                const option = document.createElement('option');
                option.value = item[codeField];
                option.textContent = item[descField];
                select.appendChild(option);
            });
        }
        
        function resetSelect(selectId) {
            const select = document.getElementById(selectId);
            select.innerHTML = '<option value="">Select ' + selectId.charAt(0).toUpperCase() + selectId.slice(1) + '...</option>';
            select.disabled = true;
        }
        
        function updateFullAddress() {
            const street = document.getElementById('street').value;
            const barangayText = document.getElementById('barangay').selectedOptions[0]?.text || '';
            const municipalityText = document.getElementById('municipality').selectedOptions[0]?.text || '';
            const provinceText = document.getElementById('province').selectedOptions[0]?.text || '';
            const regionText = document.getElementById('region').selectedOptions[0]?.text || '';
            const postalCode = document.getElementById('postalCode').value;
            
            const addressParts = [
                street,
                barangayText.startsWith('Select') ? '' : barangayText,
                municipalityText.startsWith('Select') ? '' : municipalityText,
                provinceText.startsWith('Select') ? '' : provinceText,
                regionText.startsWith('Select') ? '' : regionText,
                'PHILIPPINES',
                postalCode
            ].filter(part => part && part.trim() !== '');
            
            document.getElementById('fullAddress').value = addressParts.join(', ');
        }
        
        function updateProgress() {
            const requiredFields = ['deviceName', 'region', 'province', 'municipality', 'barangay', 'street', 'postalCode', 'email', 'password'];
            const filledFields = requiredFields.filter(fieldId => {
                const field = document.getElementById(fieldId);
                return field && field.value.trim() !== '';
            });
            
            const progress = (filledFields.length / requiredFields.length) * 100;
            document.getElementById('progressFill').style.width = progress + '%';
        }
        
        // Add input listeners for progress tracking
        document.addEventListener('input', updateProgress);
        
        document.getElementById('setupForm').addEventListener('submit', async function(e) {
            e.preventDefault();
            
            const formData = new FormData(e.target);
            const submitBtn = document.getElementById('submitBtn');
            const statusDiv = document.getElementById('status');
            
            // Add address codes to form data
            formData.append('regionCode', selectedCodes.region);
            formData.append('provinceCode', selectedCodes.province);
            formData.append('municipalityCode', selectedCodes.municipality);
            formData.append('barangayCode', selectedCodes.barangay);
            
            const data = Object.fromEntries(formData.entries());
            
            submitBtn.disabled = true;
            submitBtn.textContent = '⏳ Setting up device...';
            statusDiv.style.display = 'none';
            
            try {
                const response = await fetch('/setup', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(data)
                });
                
                const result = await response.json();
                statusDiv.style.display = 'block';
                
                if (response.ok && result.success) {
                    statusDiv.className = 'status success';
                    statusDiv.innerHTML = '✅ Setup completed successfully! Device will restart and begin monitoring.';
                    setTimeout(() => {
                        window.location.href = '/restart';
                    }, 3000);
                } else {
                    statusDiv.className = 'status error';
                    statusDiv.innerHTML = '❌ Setup failed: ' + (result.message || 'Unknown error');
                    submitBtn.disabled = false;
                    submitBtn.textContent = '🚀 Complete Setup';
                }
            } catch (error) {
                statusDiv.style.display = 'block';
                statusDiv.className = 'status error';
                statusDiv.innerHTML = '❌ Network error: ' + error.message;
                submitBtn.disabled = false;
                submitBtn.textContent = '🚀 Complete Setup';
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
    deviceStreet = doc["street"].as<String>();
    devicePostalCode = doc["postalCode"].as<String>();
    String email = doc["email"].as<String>();
    String password = doc["password"].as<String>();
    
    // Address codes
    String regionCode = doc["regionCode"].as<String>();
    String provinceCode = doc["provinceCode"].as<String>();
    String municipalityCode = doc["municipalityCode"].as<String>();
    String barangayCode = doc["barangayCode"].as<String>();
    String fullAddress = doc["fullAddress"].as<String>();
    
    Serial.println("Setup data received:");
    Serial.println("Device Name: " + deviceName);
    Serial.println("Street: " + deviceStreet);
    Serial.println("Postal Code: " + devicePostalCode);
    Serial.println("Full Address: " + fullAddress);
    
    // Store device location
    deviceLocation = fullAddress;
    
    // TODO: Complete device setup via database
    // This would involve:
    // 1. Authenticating admin user
    // 2. Creating device record with proper address structure
    // 3. Linking device to admin user
    
    markSetupCompleted();
    
    JsonDocument response;
    response["success"] = true;
    response["message"] = "Device setup completed successfully";
    response["device_id"] = deviceId;
    response["device_name"] = deviceName;
    response["full_location"] = fullAddress;
    
    String responseStr;
    serializeJson(response, responseStr);
    server->send(200, "application/json", responseStr);
  } else {
    server->send(405, "text/plain", "Method Not Allowed");
  }
}

void DeviceSetup::handleRestart() {
  server->send(200, "text/html", 
    "<html><body style='font-family: Arial; text-align: center; padding: 50px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white;'>"
    "<h1>🔄 Device Restarting...</h1>"
    "<p>Setup completed successfully. Device will now restart and begin normal operation.</p>"
    "<div style='margin-top: 30px; font-size: 14px; opacity: 0.8;'>Please wait while the device initializes...</div>"
    "</body></html>");
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
  server->on("/address_data", [this]() { handleAddressData(); });
  
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