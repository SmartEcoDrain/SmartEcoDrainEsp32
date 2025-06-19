#ifndef DEVICE_DB_H
#define DEVICE_DB_H

#include <Arduino.h>
#include "device.h"
#include "../TinyGSMSupabase/supabase.h"

class DeviceDB {
private:
  Supabase* db;

public:
  DeviceDB(Supabase* database);
  
  // Device CRUD operations
  int createDevice(const Device& device);
  int updateDevice(const Device& device);
  String getDevice(const String& deviceId);
  String getDeviceByOwnerId(const String& ownerId);
  int removeDevice(const String& deviceId);
  
  // Device data operations
  int createDeviceData(const DeviceData& deviceData);
  int updateDeviceData(const DeviceData& deviceData);
  String getDeviceData(const String& deviceId);
  String getLatestDeviceData(const String& deviceId);
  
  // Device status operations
  int updateDeviceStatus(const String& deviceId, bool isOnline);
  int updateDeviceLocation(const String& deviceId, const AddressLocation& location);
  
  // Utility functions
  Device parseDeviceFromResponse(const String& response);
  DeviceData parseDeviceDataFromResponse(const String& response);
};

#endif