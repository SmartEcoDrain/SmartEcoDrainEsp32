#ifndef ADDRESS_DB_H
#define ADDRESS_DB_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../TinyGSMSupabase/supabase.h"

struct Country {
  String code;
  String desc;
};

struct Region {
  String code;
  String desc;
};

struct Province {
  String code;
  String desc;
};

struct Municipality {
  String code;
  String desc;
};

struct Barangay {
  String code;
  String desc;
};

struct AddressLocation {
  Country country;
  Region region;
  Province province;
  Municipality municipality;
  Barangay barangay;
  String postalCode;
  String street;
  String fullAddress;
};

// Database functions using RPC
class AddressDB {
private:
  Supabase* db;

public:
  AddressDB(Supabase* database);
  
  // Use RPC function for efficient address data retrieval
  String getAddressDropdownData(const String& regCode = "", const String& provCode = "", const String& cityMunCode = "");
  
  // Individual getters (legacy support)
  String getRegions();
  String getProvinces(const String& regCode);
  String getMunicipalities(const String& provCode);
  String getBarangays(const String& cityMunCode);
  
  // Parse address from JSON
  AddressLocation parseAddressFromJSON(const JsonObject& obj);
  AddressLocation parseAddressFromJSON(const String& jsonString);
  
  // Create address JSON
  String createAddressJSON(const AddressLocation& address);
  JsonDocument createAddressJSONObject(const AddressLocation& address);
  
  // Build full address string
  String buildFullAddress(const AddressLocation& address);
};

// Utility functions
AddressLocation createAddress(const String& countryCode, const String& countryDesc,
                              const String& regionCode, const String& regionDesc,
                              const String& provinceCode, const String& provinceDesc,
                              const String& municipalityCode, const String& municipalityDesc,
                              const String& barangayCode, const String& barangayDesc,
                              const String& postalCode, const String& street);

#endif