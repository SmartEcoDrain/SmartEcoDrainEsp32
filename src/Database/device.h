
#include <Arduino.h>
#include "../configs.h"
#include "address.h"
#include <map>

struct Device {
  String id;
  String name;
  String ownerId;
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
