#pragma once

#include <Arduino.h>
#include "address.h"
#include <map>

struct User {
  String uuid;
  String fullName;
  String profileImage;
  String createdAt;
};
