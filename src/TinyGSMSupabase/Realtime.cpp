#include "Realtime.h"

int SupabaseRealtime::makeHttpRequest(String method, String url, String headers, String body)
{
  // Extract hostname and path from URL
  String hostname_only = url;
  hostname_only.replace("https://", "");
  hostname_only.replace("http://", "");
  
  int pathIndex = hostname_only.indexOf('/');
  String path = "/";
  if (pathIndex >= 0) {
    path = hostname_only.substring(pathIndex);
    hostname_only = hostname_only.substring(0, pathIndex);
  }

  // Connect to server
  if (!client->connect(hostname_only.c_str(), 443)) {
    Serial.println("Connection failed!");
    return -1;
  }

  // Send HTTP request
  client->print(method + " " + path + " HTTP/1.1\r\n");
  client->print("Host: " + hostname_only + "\r\n");
  client->print(headers);
  client->print("Connection: close\r\n");
  
  if (body.length() > 0) {
    client->print("Content-Length: " + String(body.length()) + "\r\n");
  }
  
  client->print("\r\n");
  
  if (body.length() > 0) {
    client->print(body);
  }

  // Read response
  return readHttpResponse().toInt();
}

String SupabaseRealtime::readHttpResponse()
{
  String response = "";
  int httpCode = 0;
  bool firstLine = true;
  bool headersComplete = false;

  while (client->connected() || client->available()) {
    if (client->available()) {
      String line = client->readStringUntil('\n');
      line.trim();
      
      if (firstLine && line.length() > 0) {
        int codePos = line.indexOf(' ') + 1;
        httpCode = line.substring(codePos, line.indexOf(' ', codePos)).toInt();
        firstLine = false;
      }
      
      if (!headersComplete && line.length() == 0) {
        headersComplete = true;
        continue;
      }
      
      if (headersComplete) {
        if (response.length() > 0) response += "\n";
        response += line;
      }
    }
  }
  
  client->stop();
  data = response;
  return String(httpCode);
}

int SupabaseRealtime::_login_process()
{
  JsonDocument doc;
  String url = hostname + "/auth/v1/token?grant_type=password";
  Serial.println("Beginning to login to " + url);

  String headers = "apikey: " + key + "\r\n";
  headers += "Content-Type: application/json\r\n";
  
  String query = "{\"" + loginMethod + "\": \"" + phone_or_email + "\", \"password\": \"" + password + "\"}";
  
  int httpCode = makeHttpRequest("POST", url, headers, query);

  if (httpCode > 0)
  {
    deserializeJson(doc, data);
    if (doc.containsKey("access_token") && !doc["access_token"].isNull() && doc["access_token"].is<String>() && !doc["access_token"].as<String>().isEmpty())
    {
      String USER_TOKEN = doc["access_token"].as<String>();
      authTimeout = doc["expires_in"].as<int>() * 1000;
      Serial.println("Login Success");
    }
    else
    {
      Serial.println("Login Failed: Invalid access token in response");
    }
  }
  else
  {
    Serial.print("Login Failed : ");
    Serial.println(httpCode);
  }

  loginTime = millis();
  return httpCode;
}

void SupabaseRealtime::pollForChanges(String table, String schema)
{
  if (millis() - lastPollTime < pollInterval) {
    return;
  }
  
  lastPollTime = millis();
  
  // Simple polling implementation - get recent changes
  String pollUrl = hostname + "/rest/v1/" + table + "?select=*&order=id.desc&limit=1";
  String headers = "apikey: " + key + "\r\n";
  headers += "Content-Type: application/json\r\n";
  
  if (useAuth) {
    unsigned long t_now = millis();
    if (t_now - loginTime >= authTimeout) {
      _login_process();
    }
    // Note: Would need to store USER_TOKEN as class member
    // headers += "Authorization: Bearer " + USER_TOKEN + "\r\n";
  }
  
  int httpCode = makeHttpRequest("GET", pollUrl, headers);
  
  if (httpCode == 200 && data.length() > 0) {
    // Process the response and call handler if data changed
    handler(data);
  }
}

void SupabaseRealtime::loop()
{
  // Request AUTH token every 50 minutes (on default timeout / 60 min)
  if (useAuth && millis() - loginTime > authTimeout / 1.2)
  {
    _login_process();
  }
  
  // Note: In a real implementation, you would call pollForChanges 
  // for each table you're monitoring
}

void SupabaseRealtime::begin(String hostname_param, String key_param, TinyGsm& modem_ref, TinyGsmClient& client_ref, void (*func)(String))
{
  hostname_param.replace("https://", "");
  this->hostname = "https://" + hostname_param;
  this->key = key_param;
  this->handler = func;
  this->modem = &modem_ref;
  this->client = &client_ref;
}

void SupabaseRealtime::setPollInterval(unsigned long interval_ms)
{
  pollInterval = interval_ms;
}

int SupabaseRealtime::login_email(String email_a, String password_a)
{
  useAuth = true;
  loginMethod = "email";
  phone_or_email = email_a;
  password = password_a;

  int httpCode = 0;
  while (httpCode <= 0)
  {
    httpCode = _login_process();
  }
  return httpCode;
}

int SupabaseRealtime::login_phone(String phone_a, String password_a)
{
  useAuth = true;
  loginMethod = "phone";
  phone_or_email = phone_a;
  password = password_a;

  int httpCode = 0;
  while (httpCode <= 0)
  {
    httpCode = _login_process();
  }
  return httpCode;
}