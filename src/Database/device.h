#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include "../configs.h"
#include "address.h"
#include <map>

struct Device {
  String id;
  String name;
  String ownerId;
  String deviceDataId;
  AddressLocation location;
  bool onlineStatus;
  bool isActive = true;
  String deviceVersion = DEVICE_VERSION;
  std::map<String, String> config;
  String createdAt;
  String updatedAt;
};

struct DeviceDataDevice {
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
  std::map<String, String> otherData;
  std::map<String, String> status;
};

struct DeviceDataModule {
  float tof;
  float force0;
  float force1;
  float weight;
  float turbidity;
  float ultrasonic;
  std::map<String, String> otherData;
  std::map<String, String> status;
};

struct DeviceData {
  int id;
  String deviceId;
  DeviceDataDevice device;
  DeviceDataModule module;
  String lastUpdatedAt;
};

JsonDocument createDeviceDataJSONObject(const DeviceData &deviceData);
String createDeviceDataJSON(const DeviceData &deviceData);
DeviceData parseDeviceDataJSON(const JsonObject &obj);
DeviceData parseDeviceDataJSON(const JsonDocument &doc);
String createDeviceJSON(const Device &device);
Device parseDeviceJSON(const JsonObject &obj);
Device parseDeviceJSON(const JsonDocument &doc);
Device createDevice(const String &id, const String &name, const String &ownerId,
                     const AddressLocation &location, bool onlineStatus = false,
                     bool isActive = true, const String &deviceVersion = DEVICE_VERSION);

#endif // DEVICE_DB_H