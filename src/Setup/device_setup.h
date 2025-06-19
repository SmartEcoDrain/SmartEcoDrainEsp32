#ifndef DEVICE_SETUP_H
#define DEVICE_SETUP_H

#include <Arduino.h>
#include <WebServer.h>
#include "../configs.h"
#include "../Database/device.h"
#include "../Database/user.h"
#include "../Database/address.h"
#include "../TinyGSMSupabase/supabase.h"

class DeviceSetup {
private:
  WebServer* server;
  bool setupMode;
  bool setupCompleted;
  String deviceName;
  String deviceLocation;
  String deviceStreet;
  String devicePostalCode;
  String deviceId;
  Supabase* db;

  // Web handlers
  void handleRoot();
  void handleSetup();
  void handleRestart();
  void handleAddressData();

public:
  DeviceSetup(const String& deviceId, Supabase* database);
  ~DeviceSetup();
  
  bool isSetupCompleted();
  void markSetupCompleted();
  void resetSetupFlag();
  void startSetupMode();
  void loop();
  bool checkDeviceSetupStatus();
  
  // Getters
  String getDeviceName() const { return deviceName; }
  String getDeviceLocation() const { return deviceLocation; }
  bool isInSetupMode() const { return setupMode && !setupCompleted; }
};

#endif