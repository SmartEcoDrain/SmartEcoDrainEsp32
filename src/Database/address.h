#include <Arduino.h>
#include <ArduinoJson.h>

struct LocationPart {
  String code;
  String desc;
};

struct AddressLocation {
  LocationPart country;
  LocationPart region;
  LocationPart province;
  LocationPart municipality;
  LocationPart barangay;
  String postalCode;
  String street;
  String fullAddress;
};

JsonDocument createAddressJsonObject(const AddressLocation &address);
String createAddressJSON(const AddressLocation &address);

JsonDocument parseAddressJsonObject(const String &jsonString);
AddressLocation parseAddressJSON(const String &jsonString);
AddressLocation parseAddressJSON(const JsonObject &obj);
AddressLocation parseAddressJSON(const JsonDocument &doc);

AddressLocation createAddress(const String &countryCode, const String &countryDesc,
                              const String &regionCode, const String &regionDesc,
                              const String &provinceCode, const String &provinceDesc,
                              const String &municipalityCode, const String &municipalityDesc,
                              const String &barangayCode, const String &barangayDesc,
                              const String &postalCode, const String &street);