#pragma once

#ifdef TINY_GSM_MODEM_A7670
#undef TINY_GSM_MODEM_A7670
#endif

#ifdef TINY_GSM_MODEM_A7608
#undef TINY_GSM_MODEM_A7608
#endif

// Use SecureClient
#ifndef TINY_GSM_MODEM_A76XXSSL
#define TINY_GSM_MODEM_A76XXSSL //Support A7670X/A7608X/SIM7670G
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG Serial

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#include <Arduino.h>
#include <WebServer.h>
#include "../Utils/utilities.h"
#include "../configs.h"
#include "../Database/device_db.h"
#include "../Database/address.h"
#include "../Database/profile.h" // Add this include

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
  
  // Database connections
  DeviceDB* deviceDB;
  AddressDB* addressDB;
  ProfileDB* profileDB; // Add this member
  
  // TinyGSM references for setup mode
  TinyGsm* modem;
  TinyGsmClientSecure* client;
  bool modemInitialized;

  // Web handlers
  void handleRoot();
  void handleSetup();
  void handleRestart();
  void handleAddressData();
  void handleProfileData(); // Add this method

  
  // Modem initialization
  bool initializeModemForSetup();
  void checkModemConnection();

public:
  DeviceSetup(const String& deviceId, TinyGsm* modem_ref, TinyGsmClientSecure* client_ref);
  ~DeviceSetup();
  
  bool isSetupCompleted();
  void markSetupCompleted();
  void resetSetupFlag();
  void startSetupMode();
  void loop();
  bool checkDeviceSetupStatus();
  
  // Getters
  String getDeviceName() const;
  String getDeviceLocation() const;
  bool isInSetupMode() const;
  bool isModemReady() const;
};