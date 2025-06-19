
#include <Arduino.h>

#include "utilities.h"
#include <TinyGsmClient.h>

String createTimestamp(TinyGsm &modem)
{
  // create modem stream
  Stream &modemStream = modem.stream;
  
  // send AT command to get the current time
  modem.sendAT("+CCLK?");
  if (modem.waitResponse("+CCLK:") == 1)
  {
    String datetime = modemStream.readStringUntil('\n');
    datetime.trim();
    return datetime.substring(datetime.indexOf('"') + 1, datetime.lastIndexOf('"'));
  }

  return ""; 
}