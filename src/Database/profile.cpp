#include "profile.h"
#include <ArduinoHttpClient.h>

ProfileDB::ProfileDB(TinyGsm* modem_ref, TinyGsmClientSecure* client_ref) 
  : modem(modem_ref), client(client_ref) {
}

ProfileDB::~ProfileDB() {
}

String ProfileDB::getPublicProfiles() {
  if (!modem || !client) {
    Serial.println("Modem or client not initialized");
    return "{\"success\":false,\"data\":[]}";
  }

  String endpoint = "/api/profiles/public/all";

  Serial.println("Fetching public profiles from: " + endpoint);

  HttpClient http(*client, SERVER_HOST, SERVER_PORT);

  Serial.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive(); // Currently, this is needed for HTTPS
  int err = http.get(endpoint);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    return "{\"success\":false,\"profiles\":[]}";
  }

  int statusCode = http.responseStatusCode();
  Serial.print(F("Response status code: "));
  Serial.println(statusCode);
  
  if (!statusCode) {
    return "{\"success\":false,\"profiles\":[]}";
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
    // Parse and format the response for frontend consumption
    JsonDocument doc;
    deserializeJson(doc, body);
    
    if (doc["success"].as<bool>() && doc.containsKey("profiles")) {
      return body;
    } else {
      Serial.println("Invalid response format from profiles API");
      return "{\"success\":false,\"profiles\":[]}";
    }
  } else {
    Serial.println("Failed to fetch public profiles: " + body);
    return "{\"success\":false,\"profiles\":[]}";
  }
}

ProfileInfo ProfileDB::parseProfileFromJSON(const JsonObject& obj) {
  ProfileInfo profile;
  
  profile.uuid = obj["uuid"].as<String>();
  profile.firstName = obj["first_name"].as<String>();
  profile.lastName = obj["last_name"].as<String>();
  profile.email = obj["email"].as<String>();
  profile.displayName = obj["display_name"].as<String>();
  
  // Create display name if not provided
  if (profile.displayName.isEmpty()) {
    profile.displayName = profile.firstName + " " + profile.lastName;
    profile.displayName.trim();
    if (profile.displayName.isEmpty()) {
      profile.displayName = profile.email;
    }
  }
  
  return profile;
}

ProfileInfo ProfileDB::parseProfileFromJSON(const String& jsonString) {
  JsonDocument doc;
  deserializeJson(doc, jsonString);
  return parseProfileFromJSON(doc.as<JsonObject>());
}

String ProfileDB::createProfileJSON(const ProfileInfo& profile) {
  JsonDocument doc = createProfileJSONObject(profile);
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

JsonDocument ProfileDB::createProfileJSONObject(const ProfileInfo& profile) {
  JsonDocument doc;
  
  doc["uuid"] = profile.uuid;
  doc["first_name"] = profile.firstName;
  doc["last_name"] = profile.lastName;
  doc["email"] = profile.email;
  doc["display_name"] = profile.displayName;
  
  return doc;
}

ProfileInfo createProfile(const String& uuid, const String& firstName, 
                         const String& lastName, const String& email, 
                         const String& displayName) {
  ProfileInfo profile;
  profile.uuid = uuid;
  profile.firstName = firstName;
  profile.lastName = lastName;
  profile.email = email;
  profile.displayName = displayName.isEmpty() ? firstName + " " + lastName : displayName;

  profile.displayName.trim();
  if (profile.displayName.isEmpty()) {
    profile.displayName = email;
  }
  
  return profile;
}