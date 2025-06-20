#include "device_db.h"
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>

DeviceDB::DeviceDB(TinyGsm* modem_ref, TinyGsmClientSecure* client_ref) 
  : modem(modem_ref), client(client_ref) {
}

DeviceDB::~DeviceDB() {
}

int DeviceDB::createDevice(const Device& device) {
  String deviceJson = createDeviceJSON(device);
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS POST request... "));
  
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  http.beginRequest();
  int err = http.post("/api/devices");
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  // Add required headers
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.sendHeader("Content-Length", deviceJson.length());
  http.endRequest();

  // Send the JSON payload
  http.print(deviceJson);

  int statusCode = http.responseStatusCode();
  Serial.print(F("Response status code: "));
  Serial.println(statusCode);
  
  if (!statusCode) {
    http.stop();
    return 0;
  }

  Serial.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    Serial.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    Serial.print(F("Content length is: "));
    Serial.println(length);
  }
  if (http.isResponseChunked()) {
    Serial.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  Serial.println(F("Response:"));
  Serial.println(body);

  Serial.print(F("Body length is: "));
  Serial.println(body.length());

  http.stop();
  Serial.println(F("Server disconnected"));
  
  Serial.printf("Create Device Response - Status: %d\n", statusCode);
  Serial.println("Response: " + body);
  
  return statusCode;
}

int DeviceDB::updateDevice(const Device& device) {
  String deviceJson = createDeviceJSON(device);
  String endpoint = "/api/devices?uuid=" + device.uuid;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS PUT request... "));
  
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  http.beginRequest();
  int err = http.put(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  // Add required headers
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.sendHeader("Content-Length", deviceJson.length());
  http.endRequest();

  // Send the JSON payload
  http.print(deviceJson);

  int statusCode = http.responseStatusCode();
  String body = http.responseBody();
  
  http.stop();
  Serial.println(F("Server disconnected"));
  
  return statusCode;
}

String DeviceDB::getDevice(const String& deviceId) {
  String endpoint = "/api/devices?uuid=" + deviceId;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  int err = http.get(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return "{}";
  }

  int statusCode = http.responseStatusCode();
  Serial.print(F("Response status code: "));
  Serial.println(statusCode);
  
  if (!statusCode) {
    http.stop();
    return "{}";
  }

  String body = http.responseBody();
  http.stop();
  Serial.println(F("Server disconnected"));
  
  if (statusCode == 200) {
    return body;
  }
  return "{}";
}

String DeviceDB::getDeviceByOwnerId(const String& ownerId) {
  String endpoint = "/api/devices?ownerUuid=" + ownerId;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  int err = http.get(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return "{}";
  }

  int statusCode = http.responseStatusCode();
  String body = http.responseBody();
  
  http.stop();
  Serial.println(F("Server disconnected"));
  
  if (statusCode == 200) {
    return body;
  }
  return "{}";
}

int DeviceDB::removeDevice(const String& deviceId) {
  String endpoint = "/api/devices?uuid=" + deviceId;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS DELETE request... "));
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  int err = http.del(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  http.stop();
  
  return statusCode;
}

int DeviceDB::createDeviceData(const DeviceData& deviceData) {
  String deviceDataJson = createDeviceDataJSON(deviceData);
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS POST request... "));
  
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  http.beginRequest();
  int err = http.post("/api/device-data");
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  // Add required headers
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.sendHeader("Content-Length", deviceDataJson.length());
  http.endRequest();

  // Send the JSON payload
  http.print(deviceDataJson);

  int statusCode = http.responseStatusCode();
  String body = http.responseBody();
  
  http.stop();
  Serial.println(F("Server disconnected"));
  
  Serial.printf("Create Device Data Response - Status: %d\n", statusCode);
  Serial.println("Response: " + body);
  
  return statusCode;
}

int DeviceDB::updateDeviceData(const DeviceData& deviceData) {
  String deviceDataJson = createDeviceDataJSON(deviceData);
  String endpoint = "/api/device-data?deviceId=" + deviceData.deviceId;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS PUT request... "));
  
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  http.beginRequest();
  int err = http.put(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  // Add required headers
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.sendHeader("Content-Length", deviceDataJson.length());
  http.endRequest();

  // Send the JSON payload
  http.print(deviceDataJson);

  int statusCode = http.responseStatusCode();
  http.stop();

  return statusCode;
}

String DeviceDB::getDeviceData(const String& deviceId) {
  String endpoint = "/api/device-data?deviceId=" + deviceId;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  int err = http.get(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return "{}";
  }

  int statusCode = http.responseStatusCode();
  String body = http.responseBody();
  
  http.stop();
  Serial.println(F("Server disconnected"));
  
  if (statusCode == 200) {
    return body;
  }
  return "{}";
}

String DeviceDB::getLatestDeviceData(const String& deviceId) {
  String endpoint = "/api/device-data?deviceId=" + deviceId + "&latest=true";
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  int err = http.get(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return "{}";
  }

  int statusCode = http.responseStatusCode();
  String body = http.responseBody();
  
  http.stop();
  Serial.println(F("Server disconnected"));
  
  if (statusCode == 200) {
    return body;
  }
  return "{}";
}

int DeviceDB::updateDeviceStatus(const String& deviceId, bool isOnline) {
  JsonDocument doc;
  doc["onlineStatus"] = isOnline;
  
  String updateJson;
  serializeJson(doc, updateJson);
  
  String endpoint = "/api/devices?uuid=" + deviceId;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS PUT request... "));
  
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  http.beginRequest();
  int err = http.put(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  // Add required headers
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.sendHeader("Content-Length", updateJson.length());
  http.endRequest();

  // Send the JSON payload
  http.print(updateJson);

  int statusCode = http.responseStatusCode();
  http.stop();

  return statusCode;
}

int DeviceDB::updateDeviceLocation(const String& deviceId, const AddressLocation& location) {
  JsonDocument doc;
  JsonObject locationObj = doc["location"].to<JsonObject>();
  locationObj["country"] = location.country.code;
  locationObj["region"] = location.region.code;
  locationObj["province"] = location.province.code;
  locationObj["municipality"] = location.municipality.code;
  locationObj["barangay"] = location.barangay.code;
  locationObj["postalCode"] = location.postalCode;
  locationObj["street"] = location.street;
  
  String updateJson;
  serializeJson(doc, updateJson);
  
  String endpoint = "/api/devices?uuid=" + deviceId;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS PUT request... "));
  
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  http.beginRequest();
  int err = http.put(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  // Add required headers
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.sendHeader("Content-Length", updateJson.length());
  http.endRequest();

  // Send the JSON payload
  http.print(updateJson);

  int statusCode = http.responseStatusCode();
  http.stop();

  return statusCode;
}

Device DeviceDB::parseDeviceFromResponse(const String& response) {
  JsonDocument doc;
  deserializeJson(doc, response);
  
  if (doc.is<JsonArray>() && doc.size() > 0) {
    return parseDeviceJSON(doc[0].as<JsonObject>());
  } else if (doc.is<JsonObject>()) {
    return parseDeviceJSON(doc.as<JsonObject>());
  }
  
  return Device(); // Return empty device if parsing fails
}

DeviceData DeviceDB::parseDeviceDataFromResponse(const String& response) {
  JsonDocument doc;
  deserializeJson(doc, response);
  
  if (doc.is<JsonArray>() && doc.size() > 0) {
    return parseDeviceDataJSON(doc[0].as<JsonObject>());
  } else if (doc.is<JsonObject>()) {
    return parseDeviceDataJSON(doc.as<JsonObject>());
  }
  
  return DeviceData(); // Return empty device data if parsing fails
}

int DeviceDB::authenticateUser(const String& email, const String& password) {
  JsonDocument authDoc;
  authDoc["email"] = email;
  authDoc["password"] = password;
  
  String authJson;
  serializeJson(authDoc, authJson);
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS POST request... "));
  
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  http.beginRequest();
  int err = http.post("/api/auth/login");
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  // Add required headers
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.sendHeader("Content-Length", authJson.length());
  http.endRequest();

  // Send the JSON payload
  http.print(authJson);

  int statusCode = http.responseStatusCode();
  String body = http.responseBody();
  
  http.stop();
  Serial.println(F("Server disconnected"));
  
  Serial.printf("Auth Response - Status: %d\n", statusCode);
  Serial.println("Auth Response: " + body);
  
  return statusCode;
}

String DeviceDB::checkDeviceSetup(const String& deviceId) {
  String endpoint = "/api/devices/setup-status?deviceId=" + deviceId;
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  int err = http.get(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return "{\"setup_completed\": false}";
  }

  int statusCode = http.responseStatusCode();
  String body = http.responseBody();
  
  http.stop();
  Serial.println(F("Server disconnected"));
  
  if (statusCode == 200) {
    return body;
  }
  return "{\"setup_completed\": false}";
}

int DeviceDB::sendHeartbeat(const String& deviceId) {
  JsonDocument heartbeatDoc;
  heartbeatDoc["deviceId"] = deviceId;
  heartbeatDoc["timestamp"] = millis();
  heartbeatDoc["status"] = "online";
  
  String heartbeatJson;
  serializeJson(heartbeatDoc, heartbeatJson);
  
  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS POST request... "));
  
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  http.beginRequest();
  int err = http.post("/api/heartbeat");
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return 0;
  }

  // Add required headers
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Accept", "application/json");
  http.sendHeader("User-Agent", "SmartEchoDrain/1.0");
  http.sendHeader("X-API-Key", ESP32_API_KEY);
  http.sendHeader("Content-Length", heartbeatJson.length());
  http.endRequest();

  // Send the JSON payload
  http.print(heartbeatJson);

  int statusCode = http.responseStatusCode();
  http.stop();

  return statusCode;
}