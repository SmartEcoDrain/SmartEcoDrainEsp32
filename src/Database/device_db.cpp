#include "device_db.h"
#include <ArduinoJson.h>

DeviceDB::DeviceDB(Supabase* database) : db(database) {}

int DeviceDB::createDevice(const Device& device) {
  String deviceJson = createDeviceJSON(device);
  return db->insert("devices", deviceJson, false);
}

int DeviceDB::updateDevice(const Device& device) {
  String deviceJson = createDeviceJSON(device);
  return db->update("devices").eq("id", device.id).doUpdate(deviceJson);
}

String DeviceDB::getDevice(const String& deviceId) {
  return db->from("devices").select("*").eq("id", deviceId).doSelect();
}

String DeviceDB::getDeviceByOwnerId(const String& ownerId) {
  return db->from("devices").select("*").eq("ownerId", ownerId).doSelect();
}

int DeviceDB::removeDevice(const String& deviceId) {
  // Note: TinyGSMSupabase doesn't have delete method, we'll set isActive to false
  String updateJson = "{\"isActive\": false}";
  return db->update("devices").eq("id", deviceId).doUpdate(updateJson);
}

int DeviceDB::createDeviceData(const DeviceData& deviceData) {
  String deviceDataJson = createDeviceDataJSON(deviceData);
  return db->insert("device_data", deviceDataJson, false);
}

int DeviceDB::updateDeviceData(const DeviceData& deviceData) {
  String deviceDataJson = createDeviceDataJSON(deviceData);
  if (deviceData.id > 0) {
    return db->update("device_data").eq("id", String(deviceData.id)).doUpdate(deviceDataJson);
  } else {
    return db->update("device_data").eq("deviceId", deviceData.deviceId).doUpdate(deviceDataJson);
  }
}

String DeviceDB::getDeviceData(const String& deviceId) {
  return db->from("device_data").select("*").eq("deviceId", deviceId).doSelect();
}

String DeviceDB::getLatestDeviceData(const String& deviceId) {
  return db->from("device_data").select("*").eq("deviceId", deviceId)
    .order("lastUpdatedAt", "desc", true).limit(1).doSelect();
}

int DeviceDB::updateDeviceStatus(const String& deviceId, bool isOnline) {
  String updateJson = "{\"onlineStatus\": " + String(isOnline ? "true" : "false") + "}";
  return db->update("devices").eq("id", deviceId).doUpdate(updateJson);
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
  return db->update("devices").eq("id", deviceId).doUpdate(updateJson);
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