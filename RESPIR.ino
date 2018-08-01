#include <Arduino.h>

// Needed for IR stuff
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRsend.h>
#include <FS.h>   //Include File System Headers
#include "WM.h"

#define RECV_PIN D5 //14
#define IR_LED D2 // 4  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
#define BAUD_RATE 115200
#define CAPTURE_BUFFER_SIZE 1024

#define TIMEOUT 50U  // Some A/C units have gaps in their protocols of ~40ms.
                     // e.g. Kelvinator
                     // A value this large may swallow repeats of some protocols

#define MIN_UNKNOWN_SIZE 12
// Use turn on the save buffer feature for more complete capture coverage.
  IRrecv irrecv(RECV_PIN, CAPTURE_BUFFER_SIZE, TIMEOUT, true);
  IRsend irsend(IR_LED);  // Set the GPIO to be used to sending the message.
  decode_results results;  // Somewhere to store the results
  bool irSend = true;
  unsigned long irSentTs = 0;

// Needed for WEB and flash filesystem access (SPIFFS)
//#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h>

// The section of code run only once at start-up.
void setup() {
  Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_TX_ONLY);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("RESPIR is now running and waiting for IR input on Pin ");
  Serial.println(RECV_PIN);

  // Ignore messages with less than minimum on or off pulses.
  irrecv.setUnknownThreshold(MIN_UNKNOWN_SIZE);
  irrecv.enableIRIn();  // Start the receiver
  irsend.begin();

  if (setupFS()) {
//    setupWifi();
  }

//  if (!setupWifiManagement()) {
//    Serial.println("Could not set-up Wifi management!");
//  }

}

void loop() {
  irCheck();
}

// Display the human readable state of an A/C message if we can.
void dumpACInfo(decode_results *results) {
  String description = "";
  // If we got a human-readable description of the message, display it.
  if (description != "")  Serial.println("Mesg Desc.: " + description);
}

void irCheck() {
  // Check if the IR code has been received.
  if (irrecv.decode(&results)) {
    // Display a crude timestamp.
    uint32_t now = millis();
    Serial.printf("Timestamp : %06u.%03u\n", now / 1000, now % 1000);
    if (results.overflow)
      Serial.printf("WARNING: IR code is too big for buffer (>= %d). "
                    "This result shouldn't be trusted until this is resolved. "
                    "Edit & increase CAPTURE_BUFFER_SIZE.\n",
                    CAPTURE_BUFFER_SIZE);
    // Display the basic output of what we found.
    Serial.print(resultToHumanReadableBasic(&results));
    dumpACInfo(&results);  // Display any extra A/C info if we have it.
    yield();  // Feed the WDT as the text output can take a while to print.

    // Display the library version the message was captured with.
    Serial.print("Library   : v");
    Serial.println(_IRREMOTEESP8266_VERSION_);
    Serial.println();

    // Output RAW timing info of the result.
    Serial.println(resultToTimingInfo(&results));
    yield();  // Feed the WDT (again)

    // Output the results as source code
    Serial.println(resultToSourceCode(&results));
    Serial.println("");  // Blank line between entries
    yield();  // Feed the WDT (again)

    if (now - irSentTs > 1000) {
//      sendCode(&results);
    }
  }
}

bool setupFS() {
  Serial.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    Serial.println("FS error!");
    return false;
  }

  Serial.println("FS contents:");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    Serial.print("\t");
    Serial.print(dir.fileName());
    Serial.print("\t");
    File f = dir.openFile("r");
    Serial.print(f.size());
    Serial.println(" Bytes");
    f.close();
  }

  return true;
}
