#include "user_db.h"
#include <ArduinoJson.h>

UserDB::UserDB(Supabase* database) : db(database) {}

String UserDB::getUser(const String& userId) {
  return db->from("users").select("*").eq("id", userId).doSelect();
}

String UserDB::getUserByEmail(const String& email) {
  return db->from("users").select("*").eq("email", email).doSelect();
}

String UserDB::getUserByPhone(const String& phoneNumber) {
  return db->from("users").select("*").eq("phoneNumber", phoneNumber).doSelect();
}

int UserDB::createUser(const User& user) {
  String userJson = createUserJSON(user);
  return db->insert("users", userJson, false);
}

int UserDB::updateUser(const User& user) {
  String userJson = createUserJSON(user);
  return db->update("users").eq("id", user.id).doUpdate(userJson);
}

int UserDB::loginWithEmail(const String& email, const String& password) {
  return db->login_email(email, password);
}

int UserDB::loginWithPhone(const String& phone, const String& password) {
  return db->login_phone(phone, password);
}

User UserDB::parseUserFromResponse(const String& response) {
  JsonDocument doc;
  deserializeJson(doc, response);
  
  User user;
  JsonObject obj;
  
  if (doc.is<JsonArray>() && doc.size() > 0) {
    obj = doc[0].as<JsonObject>();
  } else if (doc.is<JsonObject>()) {
    obj = doc.as<JsonObject>();
  } else {
    return user; // Return empty user if parsing fails
  }
  
  user.id = obj["id"].as<String>();
  user.email = obj["email"].as<String>();
  user.phoneNumber = obj["phoneNumber"].as<String>();
  user.age = obj["age"].as<int>();
  user.gender = obj["gender"].as<String>();
  
  // Parse name
  JsonObject nameObj = obj["name"];
  user.name.firstName = nameObj["firstName"].as<String>();
  user.name.lastName = nameObj["lastName"].as<String>();
  user.name.middleName = nameObj["middleName"].as<String>();
  user.name.suffix = nameObj["suffix"].as<String>();
  
  // Parse address - you would need to implement address parsing
  // user.address = parseAddressFromJSON(obj["address"]);
  
  return user;
}

String UserDB::createUserJSON(const User& user) {
  JsonDocument doc;
  
  doc["id"] = user.id;
  doc["email"] = user.email;
  doc["phoneNumber"] = user.phoneNumber;
  doc["age"] = user.age;
  doc["gender"] = user.gender;
  
  JsonObject nameObj = doc["name"].to<JsonObject>();
  nameObj["firstName"] = user.name.firstName;
  nameObj["lastName"] = user.name.lastName;
  nameObj["middleName"] = user.name.middleName;
  nameObj["suffix"] = user.name.suffix;
  
  // Add address object
  JsonObject addressObj = doc["address"].to<JsonObject>();
  addressObj["country"] = user.address.country.code;
  addressObj["region"] = user.address.region.code;
  addressObj["province"] = user.address.province.code;
  addressObj["municipality"] = user.address.municipality.code;
  addressObj["barangay"] = user.address.barangay.code;
  addressObj["postalCode"] = user.address.postalCode;
  addressObj["street"] = user.address.street;
  
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}