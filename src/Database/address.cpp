#include "address.h"

AddressDB::AddressDB(Supabase* database) : db(database) {}

String AddressDB::getCountries() {
  return db->from("countries").select("*").doSelect();
}

String AddressDB::getRegions(const String& countryCode) {
  if (countryCode.isEmpty()) {
    return db->from("regions").select("*").doSelect();
  }
  return db->from("regions").select("*").eq("country_code", countryCode).doSelect();
}

String AddressDB::getProvinces(const String& regionCode) {
  if (regionCode.isEmpty()) {
    return db->from("provinces").select("*").doSelect();
  }
  return db->from("provinces").select("*").eq("region_code", regionCode).doSelect();
}

String AddressDB::getMunicipalities(const String& provinceCode) {
  if (provinceCode.isEmpty()) {
    return db->from("municipalities").select("*").doSelect();
  }
  return db->from("municipalities").select("*").eq("province_code", provinceCode).doSelect();
}

String AddressDB::getBarangays(const String& municipalityCode) {
  if (municipalityCode.isEmpty()) {
    return db->from("barangays").select("*").doSelect();
  }
  return db->from("barangays").select("*").eq("municipality_code", municipalityCode).doSelect();
}

AddressLocation AddressDB::parseAddressFromJSON(const JsonObject& obj) {
  AddressLocation address;
  
  if (obj.containsKey("country")) {
    JsonObject country = obj["country"];
    address.country.code = country["code"].as<String>();
    address.country.desc = country["desc"].as<String>();
  }
  
  if (obj.containsKey("region")) {
    JsonObject region = obj["region"];
    address.region.code = region["code"].as<String>();
    address.region.desc = region["desc"].as<String>();
  }
  
  if (obj.containsKey("province")) {
    JsonObject province = obj["province"];
    address.province.code = province["code"].as<String>();
    address.province.desc = province["desc"].as<String>();
  }
  
  if (obj.containsKey("municipality")) {
    JsonObject municipality = obj["municipality"];
    address.municipality.code = municipality["code"].as<String>();
    address.municipality.desc = municipality["desc"].as<String>();
  }
  
  if (obj.containsKey("barangay")) {
    JsonObject barangay = obj["barangay"];
    address.barangay.code = barangay["code"].as<String>();
    address.barangay.desc = barangay["desc"].as<String>();
  }
  
  address.postalCode = obj["postalCode"].as<String>();
  address.street = obj["street"].as<String>();
  
  return address;
}

AddressLocation AddressDB::parseAddressFromJSON(const String& jsonString) {
  JsonDocument doc;
  deserializeJson(doc, jsonString);
  return parseAddressFromJSON(doc.as<JsonObject>());
}

String AddressDB::createAddressJSON(const AddressLocation& address) {
  JsonDocument doc = createAddressJSONObject(address);
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

JsonDocument AddressDB::createAddressJSONObject(const AddressLocation& address) {
  JsonDocument doc;
  
  JsonObject country = doc["country"].to<JsonObject>();
  country["code"] = address.country.code;
  country["desc"] = address.country.desc;
  
  JsonObject region = doc["region"].to<JsonObject>();
  region["code"] = address.region.code;
  region["desc"] = address.region.desc;
  
  JsonObject province = doc["province"].to<JsonObject>();
  province["code"] = address.province.code;
  province["desc"] = address.province.desc;
  
  JsonObject municipality = doc["municipality"].to<JsonObject>();
  municipality["code"] = address.municipality.code;
  municipality["desc"] = address.municipality.desc;
  
  JsonObject barangay = doc["barangay"].to<JsonObject>();
  barangay["code"] = address.barangay.code;
  barangay["desc"] = address.barangay.desc;
  
  doc["postalCode"] = address.postalCode;
  doc["street"] = address.street;
  
  return doc;
}

AddressLocation createAddress(const String& countryCode, const String& countryDesc,
                              const String& regionCode, const String& regionDesc,
                              const String& provinceCode, const String& provinceDesc,
                              const String& municipalityCode, const String& municipalityDesc,
                              const String& barangayCode, const String& barangayDesc,
                              const String& postalCode, const String& street) {
  AddressLocation address;
  address.country.code = countryCode;
  address.country.desc = countryDesc;
  address.region.code = regionCode;
  address.region.desc = regionDesc;
  address.province.code = provinceCode;
  address.province.desc = provinceDesc;
  address.municipality.code = municipalityCode;
  address.municipality.desc = municipalityDesc;
  address.barangay.code = barangayCode;
  address.barangay.desc = barangayDesc;
  address.postalCode = postalCode;
  address.street = street;
  return address;
}
