#include <Arduino.h>
#include "address.h"
#include <map>

struct Name {
  String suffix; // Optional, e.g., Jr., Sr.
  String lastName;
  String firstName;
  String middleName; // Optional
};

struct User {
  String id;
  Name name;
  String gender; 
  AddressLocation address;
  String phoneNumber;
  String email;
  int age;
};