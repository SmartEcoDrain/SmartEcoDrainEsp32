#include "device.h"
#include <ArduinoJson.h>

JsonDocument createDeviceDataJSONObject(const DeviceData &deviceData)
{
  JsonDocument doc;

  // Add root level fields
  doc["uuid"] = deviceData.uuid;
  doc["device_id"] = deviceData.deviceId;
  doc["last_updated_at"] = deviceData.lastUpdatedAt;
  doc["created_at"] = deviceData.createdAt;

  // Create device object
  doc["cpu_temperature"] = deviceData.cpuTemperature;
  doc["cpu_frequency"] = deviceData.cpuFrequency;
  doc["ram_usage"] = deviceData.ramUsage;
  doc["storage_usage"] = deviceData.storageUsage;
  doc["signal_strength"] = deviceData.signalStrength;
  doc["battery_voltage"] = deviceData.batteryVoltage;
  doc["battery_percentage"] = deviceData.batteryPercentage;
  doc["solar_wattage"] = deviceData.solarWattage;
  doc["uptime_ms"] = deviceData.uptimeMs;
  String batteryStatus = deviceData.batteryStatus;
  if (batteryStatus.isEmpty())
    batteryStatus = "unknown";
  doc["battery_status"] = batteryStatus;
  doc["is_online"] = deviceData.isOnline;

  // Add device other data
  for (const auto &pair : deviceData.deviceOtherData)
    doc["device_other_data"][pair.first] = pair.second;

  // Add device status
  for (const auto &pair : deviceData.deviceStatus)
    doc["device_status"][pair.first] = pair.second;

  // Create module object
  doc["tof"] = deviceData.tof;
  doc["force0"] = deviceData.force0;
  doc["force1"] = deviceData.force1;
  doc["weight"] = deviceData.weight;
  doc["turbidity"] = deviceData.turbidity;
  doc["ultrasonic"] = deviceData.ultrasonic;

  // Add module other data
  for (const auto &pair : deviceData.moduleOtherData)
    doc["module_other_data"][pair.first] = pair.second;

  // Add module status
  for (const auto &pair : deviceData.moduleStatus)
    doc["module_status"][pair.first] = pair.second;

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
    return JsonDocument();
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

  // Parse root level fields
  deviceData.uuid = obj["uuid"].as<String>();
  deviceData.deviceId = obj["device_id"].as<String>();
  deviceData.lastUpdatedAt = obj["last_updated_at"].as<String>();
  deviceData.createdAt = obj["created_at"].as<String>();

  // Parse device data
  deviceData.cpuTemperature = obj["cpu_temperature"].as<float>();
  deviceData.cpuFrequency = obj["cpu_frequency"].as<float>();
  deviceData.ramUsage = obj["ram_usage"].as<float>();
  deviceData.storageUsage = obj["storage_usage"].as<float>();
  deviceData.signalStrength = obj["signal_strength"].as<float>();
  deviceData.batteryVoltage = obj["battery_voltage"].as<float>();
  deviceData.batteryPercentage = obj["battery_percentage"].as<float>();
  deviceData.solarWattage = obj["solar_wattage"].as<float>();
  deviceData.uptimeMs = obj["uptime_ms"].as<unsigned long>();
  deviceData.batteryStatus = obj["battery_status"].as<String>();
  deviceData.isOnline = obj["is_online"].as<bool>();

  // Parse device other data
  JsonObject deviceOtherDataObj = obj["device_other_data"].as<JsonObject>();
  for (JsonPair kvp : deviceOtherDataObj)
    deviceData.deviceOtherData[kvp.key().c_str()] = kvp.value().as<String>();

  // Parse device status
  JsonObject deviceStatusObj = obj["device_status"].as<JsonObject>();
  for (JsonPair kvp : deviceStatusObj)
    deviceData.deviceStatus[kvp.key().c_str()] = kvp.value().as<String>();

  // Parse module data
  deviceData.tof = obj["tof"].as<float>();
  deviceData.force0 = obj["force0"].as<float>();
  deviceData.force1 = obj["force1"].as<float>();
  deviceData.weight = obj["weight"].as<float>();
  deviceData.turbidity = obj["turbidity"].as<float>();
  deviceData.ultrasonic = obj["ultrasonic"].as<float>();

  // Parse module other data
  JsonObject moduleOtherDataObj = obj["module_other_data"].as<JsonObject>();
  for (JsonPair kvp : moduleOtherDataObj)
    deviceData.moduleOtherData[kvp.key().c_str()] = kvp.value().as<String>();

  // Parse module status
  JsonObject moduleStatusObj = obj["module_status"].as<JsonObject>();
  for (JsonPair kvp : moduleStatusObj)
    deviceData.moduleStatus[kvp.key().c_str()] = kvp.value().as<String>();

  return deviceData;
}

DeviceData parseDeviceDataJSON(const JsonDocument &doc)
{
  return parseDeviceDataJSON(doc.as<JsonObjectConst>());
}




String createDeviceJSON(const Device &device)
{
  JsonDocument doc;

  // Create device object
  doc["uuid"] = device.uuid;
  doc["name"] = device.name;
  doc["owner_uuid"] = device.ownerUuid;
  doc["online_status"] = device.onlineStatus;
  doc["is_active"] = device.isActive;
  doc["device_version"] = device.deviceVersion;

  // Add location details as JSONB object
  JsonObject location = doc["location"].to<JsonObject>();
  location["country"] = device.location.country.code;
  location["region"] = device.location.region.code;
  location["province"] = device.location.province.code;
  location["municipality"] = device.location.municipality.code;
  location["barangay"] = device.location.barangay.code;
  location["postal_code"] = device.location.postalCode;
  location["street"] = device.location.street;

  // Add created and updated timestamps
  doc["created_at"] = device.createdAt;
  doc["updated_at"] = device.updatedAt;

  // Add dynamic config as JSONB
  JsonObject config = doc["config"].to<JsonObject>();
  for (const auto &pair : device.config)
    config[pair.first] = pair.second;

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

Device parseDeviceJSON(const JsonObject &obj)
{
  Device device;

  device.uuid = obj["uuid"].as<String>();
  device.name = obj["name"].as<String>();
  device.ownerUuid = obj["owner_uuid"].as<String>();
  device.onlineStatus = obj["online_status"].as<bool>();
  device.isActive = obj["is_active"].as<bool>();
  device.deviceVersion = obj["device_version"].as<String>();
  device.createdAt = obj["created_at"].as<String>();
  device.updatedAt = obj["updated_at"].as<String>();

  // Parse location
  JsonObject location = obj["location"].as<JsonObject>();
  device.location.country.code = location["country"].as<String>();
  device.location.region.code = location["region"].as<String>();
  device.location.province.code = location["province"].as<String>();
  device.location.municipality.code = location["municipality"].as<String>();
  device.location.barangay.code = location["barangay"].as<String>();
  device.location.postalCode = location["postalCode"].as<String>();
  device.location.street = location["street"].as<String>();

  // Parse config
  JsonObject config = obj["config"].as<JsonObject>();
  for (JsonPair kvp : config)
    device.config[kvp.key().c_str()] = kvp.value().as<String>();

  return device;
}

Device parseDeviceJSON(const JsonDocument &doc)
{
  return parseDeviceJSON(doc.as<JsonObjectConst>());
}

Device createDevice(const String &uuid, const String &name, const String &ownerUuid,
                     const AddressLocation &location, bool onlineStatus,
                     bool isActive, const String &deviceVersion)
{
  Device device;
  device.uuid = uuid;
  device.name = name;
  device.ownerUuid = ownerUuid;
  device.location = location;
  device.onlineStatus = onlineStatus;
  device.isActive = isActive;
  device.deviceVersion = deviceVersion;
  return device;
}