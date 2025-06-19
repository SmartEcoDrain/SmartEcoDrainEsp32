#include "device.h"
#include <ArduinoJson.h>

JsonDocument createDeviceDataJSONObject(const DeviceData &deviceData)
{
  JsonDocument doc;

  // Create device object with status
  JsonObject device = doc["device"].to<JsonObject>();
  device["cpu_temperature"] = deviceData.device.cpuTemperature;
  device["cpu_frequency"] = deviceData.device.cpuFrequency;
  device["ram_usage"] = deviceData.device.ramUsage;
  device["storage_usage"] = deviceData.device.storageUsage;
  device["signal_strength"] = deviceData.device.signalStrength;
  device["battery_voltage"] = deviceData.device.batteryVoltage;
  device["battery_percentage"] = deviceData.device.batteryPercentage;
  device["solar_wattage"] = deviceData.device.solarWattage;
  device["uptime_ms"] = deviceData.device.uptimeMs;
  String batteryStatus = deviceData.device.batteryStatus;
  if (batteryStatus.isEmpty())
    batteryStatus = "unknown"; // Default if not set
  device["battery_status"] = batteryStatus;
  device["is_online"] = deviceData.device.isOnline;

  JsonObject deviceStatus = device["status"].to<JsonObject>();
  for (const auto &pair : deviceData.module.status)
    deviceStatus[pair.first] = pair.second;

  // Create module object with status
  JsonObject module = doc["module"].to<JsonObject>();
  module["tof"] = deviceData.module.tof;
  module["ultrasonic"] = deviceData.module.ultrasonic;
  module["force0"] = deviceData.module.force0;
  module["force1"] = deviceData.module.force1;
  module["weight0"] = deviceData.module.weight;
  module["turbidity0"] = deviceData.module.turbidity;

  JsonObject otherData = module["otherData"].to<JsonObject>();
  for (const auto &pair : deviceData.module.otherData)
    otherData[pair.first] = pair.second;

  JsonObject moduleStatus = module["status"].to<JsonObject>();
  for (const auto &pair : deviceData.module.status)
    moduleStatus[pair.first] = pair.second;

  doc["lastUpdatedAt"] = deviceData.lastUpdatedAt;
  
  return doc;
}

String createDeviceDataJSON(const DeviceData &deviceData)
{
  JsonDocument doc = createDeviceDataJSONObject(deviceData);

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

JsonDocument parseDeviceDataJsonObject(const String &jsonString)
{
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, jsonString);
  if (error)
  {
    Serial.println("Failed to parse JSON: " + String(error.c_str()));
    return JsonDocument(); // Return empty document on error
  }

  return doc;
}

DeviceData parseDeviceDataJSON(const String &jsonString)
{
  DeviceData deviceData;
  JsonDocument doc = parseDeviceDataJsonObject(jsonString);

  return parseDeviceDataJSON(doc);
}

DeviceData parseDeviceDataJSON(const JsonObject &obj)
{
  DeviceData deviceData;

  JsonObject device = obj["device"].as<JsonObject>();
  deviceData.device.cpuTemperature = device["cpu_temperature"].as<float>();
  deviceData.device.cpuFrequency = device["cpu_frequency"].as<float>();
  deviceData.device.ramUsage = device["ram_usage"].as<float>();
  deviceData.device.storageUsage = device["storage_usage"].as<float>();
  deviceData.device.signalStrength = device["signal_strength"].as<float>();
  deviceData.device.batteryVoltage = device["battery_voltage"].as<float>();
  deviceData.device.batteryPercentage = device["battery_percentage"].as<float>();
  deviceData.device.solarWattage = device["solar_wattage"].as<float>();
  deviceData.device.uptimeMs = device["uptime_ms"].as<unsigned long>();
  deviceData.device.batteryStatus = device["battery_status"].as<String>();
  deviceData.device.isOnline = device["is_online"].as<bool>();

  JsonObject statusObj = obj["status"].as<JsonObject>();
  for (JsonPair kvp : statusObj)
    deviceData.device.status[kvp.key().c_str()] = kvp.value().as<String>();

  JsonObject module = obj["module"].as<JsonObject>();
  deviceData.module.tof = module["tof"].as<float>();
  deviceData.module.ultrasonic = module["ultrasonic"].as<float>();
  deviceData.module.force0 = module["force0"].as<float>();
  deviceData.module.force1 = module["force1"].as<float>();
  deviceData.module.weight = module["weight0"].as<float>();
  deviceData.module.turbidity = module["turbidity0"].as<float>();

  JsonObject otherDataObj = module["otherData"].as<JsonObject>();
  for (JsonPair kvp : otherDataObj)
    deviceData.module.otherData[kvp.key().c_str()] = kvp.value().as<String>();

  JsonObject moduleStatusObj = module["status"].as<JsonObject>();
  for (JsonPair kvp : moduleStatusObj)
    deviceData.module.status[kvp.key().c_str()] = kvp.value().as<String>();

  deviceData.lastUpdatedAt = obj["lastUpdatedAt"].as<String>();
  deviceData.id = obj["id"].as<int>();
  deviceData.deviceId = obj["deviceId"].as<String>();
  return deviceData;
}

DeviceData parseDeviceDataJSON(const JsonDocument &doc)
{
  return parseDeviceDataJSON(doc["device"].as<JsonObjectConst>());
}

String createDeviceJSON(const Device &device)
{
  JsonDocument doc;

  // Create device object
  JsonObject dev = doc["device"].to<JsonObject>();
  dev["id"] = device.id;
  dev["name"] = device.name;
  dev["ownerId"] = device.ownerId;
  dev["onlineStatus"] = device.onlineStatus;
  dev["isActive"] = device.isActive;
  dev["deviceVersion"] = device.deviceVersion;

  // Add location details
  JsonObject location = dev["location"].to<JsonObject>();
  location["country"] = device.location.country.code;
  location["region"] = device.location.region.code;
  location["province"] = device.location.province.code;
  location["municipality"] = device.location.municipality.code;
  location["barangay"] = device.location.barangay.code;
  location["postalCode"] = device.location.postalCode;
  location["street"] = device.location.street;

  // Add created and updated timestamps
  dev["createdAt"] = device.createdAt;
  dev["updatedAt"] = device.updatedAt;

  // Add dynamic config
  JsonObject config = dev["config"].to<JsonObject>();
  for (const auto &pair : device.config)
    config[pair.first] = pair.second;

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}