#ifndef USER_DB_H
#define USER_DB_H

#include <Arduino.h>
#include "user.h"
#include "../TinyGSMSupabase/supabase.h"

class UserDB {
private:
  Supabase* db;

public:
  UserDB(Supabase* database);
  
  // User operations
  String getUser(const String& userId);
  String getUserByEmail(const String& email);
  String getUserByPhone(const String& phoneNumber);
  int createUser(const User& user);
  int updateUser(const User& user);
  
  // Authentication
  int loginWithEmail(const String& email, const String& password);
  int loginWithPhone(const String& phone, const String& password);
  
  // Utility functions
  User parseUserFromResponse(const String& response);
  String createUserJSON(const User& user);
};

#endif