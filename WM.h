#ifndef WM_h
#define WM_h

#include "Arduino.h"
#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

//flag for saving data
static bool shouldSaveConfig = false;
  
class WM {
  public:

  //define your default values here, if there are different values in config.json, they are overwritten.
  char mqtt_server[40];
  char mqtt_port[6] = "8080";
  char blynk_token[34] = "YOUR_BLYNK_TOKEN";


//  void saveConfigCallback();
  void begin();
};



#endif
