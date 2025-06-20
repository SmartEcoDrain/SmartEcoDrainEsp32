#pragma once

#include "../configs.h"
#include <Arduino.h>
#include "address.h"
#include <map>

struct Device {
  String uuid;  // Changed from id to uuid to match database
  String ownerUuid;  // Changed from ownerId to ownerUuid
  String name;
  AddressLocation location;
  bool onlineStatus;
  bool isActive = true;
  String deviceVersion = DEVICE_VERSION;
  std::map<String, String> config;
  String createdAt;
  String updatedAt;
};

// Combined struct that matches the flattened database schema
struct DeviceData {
  String uuid;        // device_data.uuid
  String deviceId;    // device_data.device_id (references devices.uuid)
  
  // Device data fields
  float cpuTemperature;
  float cpuFrequency;
  float ramUsage;
  float storageUsage;
  float signalStrength;
  float batteryVoltage;
  float batteryPercentage;
  float solarWattage;
  unsigned long uptimeMs;
  String batteryStatus;
  bool isOnline = false;
  std::map<String, String> deviceOtherData;  // device_other_data JSONB
  std::map<String, String> deviceStatus;     // device_status JSONB
  
  // Module data fields
  float tof;
  float force0;
  float force1;
  float weight;
  float turbidity;
  float ultrasonic;
  std::map<String, String> moduleOtherData;  // module_other_data JSONB
  std::map<String, String> moduleStatus;     // module_status JSONB
  
  String lastUpdatedAt;
  String createdAt;
};

JsonDocument createDeviceDataJSONObject(const DeviceData &deviceData);
String createDeviceDataJSON(const DeviceData &deviceData);
DeviceData parseDeviceDataJSON(const JsonObject &obj);
DeviceData parseDeviceDataJSON(const JsonDocument &doc);
String createDeviceJSON(const Device &device);
Device parseDeviceJSON(const JsonObject &obj);
Device parseDeviceJSON(const JsonDocument &doc);
Device createDevice(const String &uuid, const String &name, const String &ownerUuid,
                     const AddressLocation &location, bool onlineStatus = false,
                     bool isActive = true, const String &deviceVersion = DEVICE_VERSION);
