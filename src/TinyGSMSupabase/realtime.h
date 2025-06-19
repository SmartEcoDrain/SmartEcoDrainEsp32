#ifndef REALTIME_H
#define REALTIME_H

#include <Arduino.h>
#include <ArduinoJson.h>

// TinyGSM includes instead of WiFi
#include "../Utils/utilities.h"
#include <TinyGsmClient.h>

// Note: WebSockets are not supported with TinyGSM as it requires more complex handling
// This is a simplified version that removes WebSocket functionality
// For real-time features, consider using HTTP polling instead

class SupabaseRealtime
{
private:
  String key;
  String hostname;

  // RLS Stuff
  String phone_or_email;
  String password;
  String data;
  String loginMethod;
  bool useAuth;
  int _login_process();
  unsigned int authTimeout = 0;
  unsigned long loginTime;

  // TinyGSM client references
  TinyGsmClient* client;
  TinyGsm* modem;

  // Helper methods for HTTP requests
  int makeHttpRequest(String method, String url, String headers, String body = "");
  String readHttpResponse();

  // Polling configuration
  unsigned long lastPollTime = 0;
  unsigned long pollInterval = 5000; // 5 seconds default

  std::function<void(String)> handler;

public:
  SupabaseRealtime() {}
  
  // Modified to use TinyGSM
  void begin(String hostname, String key, TinyGsm& modem_ref, TinyGsmClient& client_ref, void (*func)(String));
  
  // Polling-based methods instead of WebSocket
  void pollForChanges(String table, String schema = "public");
  void loop(); // Use polling instead of WebSocket events
  
  // Authentication methods
  int login_email(String email_a, String password_a);
  int login_phone(String phone_a, String password_a);
  
  // Configuration
  void setPollInterval(unsigned long interval_ms);
};

#endif