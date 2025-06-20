#pragma once


#include "../configs.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <TinyGsmClient.h>


struct ProfileInfo {
  String uuid;
  String firstName;
  String lastName;
  String email;
  String displayName;
};

class ProfileDB {
private:
  TinyGsm* modem;
  TinyGsmClientSecure* client;

public:
  ProfileDB(TinyGsm* modem_ref, TinyGsmClientSecure* client_ref);
  ~ProfileDB();
  
  // Get public profiles for dropdown selection
  String getPublicProfiles();
  
  // Parse profile data from JSON
  ProfileInfo parseProfileFromJSON(const JsonObject& obj);
  ProfileInfo parseProfileFromJSON(const String& jsonString);
  
  // Create profile JSON
  String createProfileJSON(const ProfileInfo& profile);
  JsonDocument createProfileJSONObject(const ProfileInfo& profile);
};

// Helper function to create profile info
ProfileInfo createProfile(const String& uuid, const String& firstName, 
                         const String& lastName, const String& email, 
                         const String& displayName = "");
