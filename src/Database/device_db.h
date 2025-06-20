#pragma once

#include <Arduino.h>
#include "device.h"
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

class DeviceDB {
private:
  TinyGsm* modem;
  TinyGsmClientSecure* client;
  HttpClient* httpClient;

public:
  DeviceDB(TinyGsm* modem_ref, TinyGsmClientSecure* client_ref);
  ~DeviceDB();
  
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
  
  // Authentication and setup
  int authenticateUser(const String& email, const String& password);
  String checkDeviceSetup(const String& deviceId);
  int sendHeartbeat(const String& deviceId);
  
  // Utility functions
  Device parseDeviceFromResponse(const String& response);
  DeviceData parseDeviceDataFromResponse(const String& response);
};
