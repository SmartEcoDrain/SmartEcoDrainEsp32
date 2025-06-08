#include "Supabase.h"

void Supabase::_check_last_string()
{
  if (url_query.charAt(url_query.length() - 1) != '&' && url_query.charAt(url_query.length() - 1) != '?')
  {
    url_query += "&";
  }
}

int Supabase::makeHttpRequest(String method, String url, String headers, String body)
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

String Supabase::readHttpResponse()
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

int Supabase::_login_process()
{
  JsonDocument doc;
  Serial.println("Beginning to login..");

  String loginUrl = hostname + "/auth/v1/token?grant_type=password";
  String headers = "apikey: " + key + "\r\n";
  headers += "Content-Type: application/json\r\n";
  
  String query = "{\"" + loginMethod + "\": \"" + phone_or_email + "\", \"password\": \"" + password + "\"}";
  
  int httpCode = makeHttpRequest("POST", loginUrl, headers, query);

  if (httpCode > 0)
  {
    deserializeJson(doc, data);
    if (doc.containsKey("access_token") && !doc["access_token"].isNull() && doc["access_token"].is<String>() && !doc["access_token"].as<String>().isEmpty())
    {
      USER_TOKEN = doc["access_token"].as<String>();
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
    Serial.println(phone_or_email);
    Serial.println(password);
    Serial.print("Login Failed : ");
    Serial.println(httpCode);
  }

  loginTime = millis();
  return httpCode;
}

// Modified begin function to accept TinyGsm reference and client
void Supabase::begin(String hostname_a, String key_a, TinyGsm& modem_ref, TinyGsmClient& client_ref)
{
  modem = &modem_ref;
  client = &client_ref;
  hostname = hostname_a;
  key = key_a;
}

String Supabase::getQuery()
{
  String temp = url_query;
  urlQuery_reset();
  return hostname + "/rest/v1/" + temp;
}

// query reset
void Supabase::urlQuery_reset()
{
  url_query = "";
}

// membuat Query Builder
Supabase &Supabase::from(String table)
{
  url_query += (table + "?");
  return *this;
}

int Supabase::insert(String table, String json, bool upsert)
{
  String insertUrl = hostname + "/rest/v1/" + table;
  String headers = "apikey: " + key + "\r\n";
  headers += "Content-Type: application/json\r\n";
  
  String preferHeader = "return=representation";
  if (upsert)
  {
    preferHeader += ",resolution=merge-duplicates";
  }
  headers += "Prefer: " + preferHeader + "\r\n";

  if (useAuth)
  {
    unsigned long t_now = millis();
    if (t_now - loginTime >= authTimeout)
    {
      _login_process();
    }
    headers += "Authorization: Bearer " + USER_TOKEN + "\r\n";
  }
  
  return makeHttpRequest("POST", insertUrl, headers, json);
}

Supabase &Supabase::select(String colls)
{
  url_query += ("select=" + colls);
  return *this;
}

Supabase &Supabase::update(String table)
{
  url_query += (table + "?");
  return *this;
}

int Supabase::upload(String bucket, String filename, String mime_type, uint8_t *buffer, uint32_t size)
{
  // Extract hostname without protocol
  String hostname_only = hostname;
  hostname_only.replace("https://", "");
  hostname_only.replace("http://", "");

  String finalPath = "/storage/v1/object/" + bucket + "/" + filename;

  // Connect to server
  if (!client->connect(hostname_only.c_str(), 443)) {
    Serial.println("Connection failed!");
    return -1;
  }

  // Create multipart form data
  String boundary = "esp32-supabase-boundary";

  String httpMainHeader = "POST " + finalPath + " HTTP/1.1\r\n";
  httpMainHeader += "Host: " + hostname_only + "\r\n";
  httpMainHeader += "apikey: " + key + "\r\n";
  httpMainHeader += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";

  String dataHeader = "--" + boundary + "\r\n";
  dataHeader += "Content-Disposition: form-data; name=\"file\"; filename=\"" + filename + "\"\r\n";
  dataHeader += "Content-Type: " + mime_type + "\r\n\r\n";

  String endingHeader = "\r\n--" + boundary + "--\r\n\r\n";

  uint32_t contentLength = dataHeader.length() + size + endingHeader.length();
  httpMainHeader += "Content-Length: " + String(contentLength) + "\r\n\r\n";

  // Send headers
  client->print(httpMainHeader);
  client->print(dataHeader);

  // Send buffer data
  client->write(buffer, size);

  // Send ending
  client->print(endingHeader);

  // Read response
  return readHttpResponse().toInt();
}

int Supabase::upload(String bucket, String filename, String mime_type, Stream *stream, uint32_t size)
{
  // Create buffer and read from stream
  uint8_t* buffer = new uint8_t[size];
  stream->readBytes(buffer, size);
  
  int result = upload(bucket, filename, mime_type, buffer, size);
  
  delete[] buffer;
  return result;
}

// Comparison Operator
Supabase &Supabase::eq(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=eq." + conditions);
  return *this;
}

Supabase &Supabase::gt(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=gt." + conditions);
  return *this;
}

Supabase &Supabase::gte(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=gte." + conditions);
  return *this;
}

Supabase &Supabase::lt(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=lt." + conditions);
  return *this;
}

Supabase &Supabase::lte(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=lte." + conditions);
  return *this;
}

Supabase &Supabase::neq(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=neq." + conditions);
  return *this;
}

Supabase &Supabase::in(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=in.(" + conditions + ")");
  return *this;
}

Supabase &Supabase::is(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=is." + conditions);
  return *this;
}

Supabase &Supabase::cs(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=cs.{" + conditions + "}");
  return *this;
}

Supabase &Supabase::cd(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=cd.{" + conditions + "}");
  return *this;
}

Supabase &Supabase::ov(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=ov.{" + conditions + "}");
  return *this;
}

Supabase &Supabase::sl(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=sl.(" + conditions + ")");
  return *this;
}

Supabase &Supabase::sr(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=sr.(" + conditions + ")");
  return *this;
}

Supabase &Supabase::nxr(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=nxr.(" + conditions + ")");
  return *this;
}

Supabase &Supabase::nxl(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=nxl.(" + conditions + ")");
  return *this;
}

Supabase &Supabase::adj(String coll, String conditions)
{
  _check_last_string();
  url_query += (coll + "=adj.(" + conditions + ")");
  return *this;
}

// Ordering
Supabase &Supabase::order(String coll, String by, bool nulls)
{
  String subq[] = {"nullsfirst", "nullslast"};
  _check_last_string();
  url_query += ("order=" + coll + "." + by + "." + subq[(int)nulls]);
  return *this;
}

Supabase &Supabase::limit(unsigned int by)
{
  _check_last_string();
  url_query += ("limit=" + String(by));
  return *this;
}

Supabase &Supabase::offset(int by)
{
  _check_last_string();
  url_query += ("offset=" + String(by));
  return *this;
}

// do select. execute this after building your query
String Supabase::doSelect()
{
  String selectUrl = hostname + "/rest/v1/" + url_query;
  String headers = "apikey: " + key + "\r\n";
  headers += "Content-Type: application/json\r\n";

  if (useAuth)
  {
    unsigned long t_now = millis();
    if (t_now - loginTime >= authTimeout)
    {
      _login_process();
    }
    headers += "Authorization: Bearer " + USER_TOKEN + "\r\n";
  }

  makeHttpRequest("GET", selectUrl, headers);
  urlQuery_reset();
  return data;
}

// do update. execute this after querying your update
int Supabase::doUpdate(String json)
{
  String updateUrl = hostname + "/rest/v1/" + url_query;
  String headers = "apikey: " + key + "\r\n";
  headers += "Content-Type: application/json\r\n";
  
  if (useAuth)
  {
    unsigned long t_now = millis();
    if (t_now - loginTime >= authTimeout)
    {
      _login_process();
    }
    headers += "Authorization: Bearer " + USER_TOKEN + "\r\n";
  }
  
  int httpCode = makeHttpRequest("PATCH", updateUrl, headers, json);
  urlQuery_reset();
  return httpCode;
}

int Supabase::login_email(String email_a, String password_a)
{
  useAuth = true;
  loginMethod = "email";
  phone_or_email = email_a;
  password = password_a;

  return _login_process();
}

int Supabase::login_phone(String phone_a, String password_a)
{
  useAuth = true;
  loginMethod = "phone";
  phone_or_email = phone_a;
  password = password_a;

  return _login_process();
}

String Supabase::rpc(String func_name, String json_param)
{
  String rpcUrl = hostname + "/rest/v1/rpc/" + func_name;
  String headers = "apikey: " + key + "\r\n";
  headers += "Content-Type: application/json\r\n";

  if (useAuth)
  {
    unsigned long t_now = millis();
    if (t_now - loginTime >= authTimeout)
    {
      _login_process();
    }
    headers += "Authorization: Bearer " + USER_TOKEN + "\r\n";
  }

  makeHttpRequest("POST", rpcUrl, headers, json_param);
  return data;
}