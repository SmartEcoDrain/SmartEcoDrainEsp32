#include "address.h"

AddressDB::AddressDB(Supabase* database) : db(database) {}

String AddressDB::getAddressDropdownData(const String& regCode, const String& provCode, const String& cityMunCode) {
  JsonDocument params;
  
  if (regCode.isEmpty()) {
    params["target_reg_code"] = nullptr;
  } else {
    params["target_reg_code"] = regCode;
  }
  
  if (provCode.isEmpty()) {
    params["target_prov_code"] = nullptr;
  } else {
    params["target_prov_code"] = provCode;
  }
  
  if (cityMunCode.isEmpty()) {
    params["target_citymun_code"] = nullptr;
  } else {
    params["target_citymun_code"] = cityMunCode;
  }
  
  String paramsStr;
  serializeJson(params, paramsStr);
  
  return db->rpc("get_address_dropdown_data", paramsStr);
}

String AddressDB::getRegions() {
  String result = getAddressDropdownData();
  
  JsonDocument doc;
  deserializeJson(doc, result);
  
  if (doc.containsKey("regions")) {
    String regionsJson;
    serializeJson(doc["regions"], regionsJson);
    return regionsJson;
  }
  
  return "[]";
}

String AddressDB::getProvinces(const String& regCode) {
  String result = getAddressDropdownData(regCode);
  
  JsonDocument doc;
  deserializeJson(doc, result);
  
  if (doc.containsKey("provinces")) {
    String provincesJson;
    serializeJson(doc["provinces"], provincesJson);
    return provincesJson;
  }
  
  return "[]";
}

String AddressDB::getMunicipalities(const String& provCode) {
  String result = getAddressDropdownData("", provCode);
  
  JsonDocument doc;
  deserializeJson(doc, result);
  
  if (doc.containsKey("cities")) {
    String citiesJson;
    serializeJson(doc["cities"], citiesJson);
    return citiesJson;
  }
  
  return "[]";
}

String AddressDB::getBarangays(const String& cityMunCode) {
  String result = getAddressDropdownData("", "", cityMunCode);
  
  JsonDocument doc;
  deserializeJson(doc, result);
  
  if (doc.containsKey("barangays")) {
    String barangaysJson;
    serializeJson(doc["barangays"], barangaysJson);
    return barangaysJson;
  }
  
  return "[]";
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
  address.fullAddress = obj["fullAddress"].as<String>();
  
  // Build full address if not provided
  if (address.fullAddress.isEmpty()) {
    address.fullAddress = buildFullAddress(address);
  }
  
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
  doc["fullAddress"] = address.fullAddress.isEmpty() ? buildFullAddress(address) : address.fullAddress;
  
  return doc;
}

String AddressDB::buildFullAddress(const AddressLocation& address) {
  String fullAddr = "";
  
  if (!address.street.isEmpty()) {
    fullAddr += address.street;
  }
  
  if (!address.barangay.desc.isEmpty()) {
    if (!fullAddr.isEmpty()) fullAddr += ", ";
    fullAddr += address.barangay.desc;
  }
  
  if (!address.municipality.desc.isEmpty()) {
    if (!fullAddr.isEmpty()) fullAddr += ", ";
    fullAddr += address.municipality.desc;
  }
  
  if (!address.province.desc.isEmpty()) {
    if (!fullAddr.isEmpty()) fullAddr += ", ";
    fullAddr += address.province.desc;
  }
  
  if (!address.region.desc.isEmpty()) {
    if (!fullAddr.isEmpty()) fullAddr += ", ";
    fullAddr += address.region.desc;
  }
  
  if (!fullAddr.isEmpty()) fullAddr += ", ";
  fullAddr += "PHILIPPINES";
  
  if (!address.postalCode.isEmpty()) {
    fullAddr += " " + address.postalCode;
  }
  
  return fullAddr;
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
  
  // Build full address
  AddressDB tempDB(nullptr);
  address.fullAddress = tempDB.buildFullAddress(address);
  
  return address;
}