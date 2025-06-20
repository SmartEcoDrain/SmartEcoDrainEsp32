#include "address.h"
#include <ArduinoHttpClient.h>

AddressDB::AddressDB(TinyGsm* modem_ref, TinyGsmClientSecure* client_ref) 
  : modem(modem_ref), client(client_ref) {
}

AddressDB::~AddressDB() {
}

String AddressDB::getAddressDropdownData(const String& regCode, const String& provCode, const String& cityMunCode) {
  if (!modem || !client) {
    Serial.println("Modem or client not initialized");
    return "{}";
  }

  String endpoint = "/api/address";
  String params = "";

  // Build query parameters
  if (!regCode.isEmpty()) {
    params += "reg_code=" + regCode;
  }
  if (!provCode.isEmpty()) {
    if (!params.isEmpty()) params += "&";
    params += "prov_code=" + provCode;
  }
  if (!cityMunCode.isEmpty()) {
    if (!params.isEmpty()) params += "&";
    params += "citymun_code=" + cityMunCode;
  }
  
  if (!params.isEmpty()) {
    endpoint += "?" + params;
  }

  Serial.println("Fetching address data from: " + endpoint);

  HttpClient http(*client, "smart-echodrain.vercel.app", 443);

  Serial.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  int err = http.get(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return "{\"success\":false,\"data\":{\"regions\":[],\"provinces\":[],\"cities\":[],\"barangays\":[]}}";
  }

  int statusCode = http.responseStatusCode();
  Serial.print(F("Response status code: "));
  Serial.println(statusCode);
  
  if (!statusCode) {
    return "{\"success\":false,\"data\":{\"regions\":[],\"provinces\":[],\"cities\":[],\"barangays\":[]}}";
  }

  Serial.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    Serial.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    Serial.print(F("Content length is: "));
    Serial.println(length);
  }
  if (http.isResponseChunked()) {
    Serial.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  Serial.println(F("Response:"));
  Serial.println(body);

  Serial.print(F("Body length is: "));
  Serial.println(body.length());

  http.stop();
  Serial.println(F("Server disconnected"));
  
  if (statusCode == 200) {
    return body;
  } else {
    Serial.println("Failed to fetch address data: " + body);
    return "{\"success\":false,\"data\":{\"regions\":[],\"provinces\":[],\"cities\":[],\"barangays\":[]}}";
  }
}

String AddressDB::getRegions() {
  String result = getAddressDropdownData();
  
  JsonDocument doc;
  deserializeJson(doc, result);
  
  // Check if response has success flag and data structure
  if (doc["success"].as<bool>() && doc.containsKey("data") && doc["data"].containsKey("regions")) {
    String regionsJson;
    serializeJson(doc["data"]["regions"], regionsJson);
    return regionsJson;
  }
  
  return "[]";
}

String AddressDB::getProvinces(const String& regCode) {
  String result = getAddressDropdownData(regCode);
  
  JsonDocument doc;
  deserializeJson(doc, result);
  
  // Check if response has success flag and data structure
  if (doc["success"].as<bool>() && doc.containsKey("data") && doc["data"].containsKey("provinces")) {
    String provincesJson;
    serializeJson(doc["data"]["provinces"], provincesJson);
    return provincesJson;
  }
  
  return "[]";
}

String AddressDB::getMunicipalities(const String& provCode) {
  String result = getAddressDropdownData("", provCode);
  
  JsonDocument doc;
  deserializeJson(doc, result);
  
  // Check if response has success flag and data structure
  if (doc["success"].as<bool>() && doc.containsKey("data") && doc["data"].containsKey("cities")) {
    String citiesJson;
    serializeJson(doc["data"]["cities"], citiesJson);
    return citiesJson;
  }
  
  return "[]";
}

String AddressDB::getBarangays(const String& cityMunCode) {
  String result = getAddressDropdownData("", "", cityMunCode);
  
  JsonDocument doc;
  deserializeJson(doc, result);
  
  // Check if response has success flag and data structure
  if (doc["success"].as<bool>() && doc.containsKey("data") && doc["data"].containsKey("barangays")) {
    String barangaysJson;
    serializeJson(doc["data"]["barangays"], barangaysJson);
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
  
  address.postalCode = obj["postal_code"].as<String>();
  address.street = obj["street"].as<String>();
  address.fullAddress = obj["full_address"].as<String>();
  
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

  doc["postal_code"] = address.postalCode;
  doc["street"] = address.street;
  doc["full_address"] = address.fullAddress.isEmpty() ? buildFullAddress(address) : address.fullAddress;

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
  return address;
}