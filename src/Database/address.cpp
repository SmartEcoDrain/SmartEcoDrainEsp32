#include "address.h"

JsonDocument createAddressJsonObject(const AddressLocation &address)
{
  JsonDocument doc;

  // Create address object
  JsonObject addr = doc["address"].to<JsonObject>();
  JsonObject country = addr["country"].to<JsonObject>();
  country["code"] = address.country.code;
  country["desc"] = address.country.desc;
  JsonObject region = addr["region"].to<JsonObject>();
  region["code"] = address.region.code;
  region["desc"] = address.region.desc;
  JsonObject province = addr["province"].to<JsonObject>();
  province["code"] = address.province.code;
  province["desc"] = address.province.desc;
  JsonObject municipality = addr["municipality"].to<JsonObject>();
  municipality["code"] = address.municipality.code;
  municipality["desc"] = address.municipality.desc;
  JsonObject barangay = addr["barangay"].to<JsonObject>();
  barangay["code"] = address.barangay.code;
  barangay["desc"] = address.barangay.desc;
  addr["postalCode"] = address.postalCode;
  addr["street"] = address.street;

  return doc;
}

String createAddressJSON(const AddressLocation &address)
{
  JsonDocument doc = createAddressJsonObject(address);

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

JsonDocument parseAddressJsonObject(const String &jsonString)
{
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, jsonString);
  if (error)
  {
    Serial.println("Failed to parse JSON: " + String(error.c_str()));
    return JsonDocument(); // Return empty document on error
  }

  return doc;
}

AddressLocation parseAddressJSON(const String &jsonString)
{
  AddressLocation address;
  JsonDocument doc = parseAddressJsonObject(jsonString);

  return parseAddressJSON(doc);
}

AddressLocation parseAddressJSON(const JsonObject &obj)
{
  AddressLocation address;

  address.country.code = obj["country"]["code"].as<String>();
  address.country.desc = obj["country"]["desc"].as<String>();
  address.region.code = obj["region"]["code"].as<String>();
  address.region.desc = obj["region"]["desc"].as<String>();
  address.province.code = obj["province"]["code"].as<String>();
  address.province.desc = obj["province"]["desc"].as<String>();
  address.municipality.code = obj["municipality"]["code"].as<String>();
  address.municipality.desc = obj["municipality"]["desc"].as<String>();
  address.barangay.code = obj["barangay"]["code"].as<String>();
  address.barangay.desc = obj["barangay"]["desc"].as<String>();
  address.postalCode = obj["postalCode"].as<String>();
  address.street = obj["street"].as<String>();

  return address;
}

AddressLocation parseAddressJSON(const JsonDocument &doc)
{
  return parseAddressJSON(doc["address"].as<JsonObjectConst>());
}

AddressLocation createAddress(const String &countryCode, const String &countryDesc,
                              const String &regionCode, const String &regionDesc,
                              const String &provinceCode, const String &provinceDesc,
                              const String &municipalityCode, const String &municipalityDesc,
                              const String &barangayCode, const String &barangayDesc,
                              const String &postalCode, const String &street)
{
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
