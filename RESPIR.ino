// comment TEST_RELIABILITY for normal use
//#define TEST_RELIABILITY

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
#define IR_SEND_KHz 38

#define TIMEOUT 15U  // Some A/C units have gaps in their protocols of ~40ms.
                     // e.g. Kelvinator
                     // A value this large may swallow repeats of some protocols

#define MIN_UNKNOWN_SIZE 12

// Initialization

// Use turn on the save buffer feature for more complete capture coverage.
IRrecv irrecv(RECV_PIN, CAPTURE_BUFFER_SIZE, TIMEOUT, true);
IRsend irsend(IR_LED);  // Set the GPIO to be used to sending the message.
decode_results results;  // Somewhere to store the results

#ifdef TEST_RELIABILITY
#include <Ticker.h>
Ticker tester;
decode_results resultsTest;
void testReliability();
#endif

// The section of code run only once at start-up.
void setup() {
  Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_TX_ONLY);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("RESPIR is now running and waiting for IR input on Pin ");
  Serial.println(RECV_PIN);

  irsend.begin();

  if (setupFS(false)) {
//    setupWifi();
  }

//  if (!setupWifiManagement()) {
//    Serial.println("Could not set-up Wifi management!");
//  }

  // Ignore messages with less than minimum on or off pulses.
  irrecv.setUnknownThreshold(MIN_UNKNOWN_SIZE);
  irrecv.enableIRIn();  // Start the receiver
}

void loop() {
  irReceiveLoop();
}

long receiveCounter = 0;
long receiveAndReSendHash = 0;

void irReceiveLoop() {
  // Check if the IR code has been received.
  if (irrecv.decode(&results)) {
    receiveCounter ++;
#ifdef TEST_RELIABILITY
    // first received code - save-it for reliability test
    if (receiveCounter == 1) {
      resultsTest.value = results.value;
      resultsTest.rawlen = results.rawlen;
      resultsTest.rawbuf = (uint16_t *)malloc(sizeof(uint16_t) * resultsTest.rawlen);
      memcpy((uint16_t *)resultsTest.rawbuf, (uint16_t *)results.rawbuf, results.rawlen);
      Serial.println("done saving buffers for testing");
      tester.attach(2, testReliability);
    }
#endif
    // Display a crude timestamp.
    uint32_t now = millis();
    const char * separator = "===========================";
    Serial.printf("%s RECEIVED IR CODE #%3d %s%s%s%s\r\n", separator, receiveCounter,
      separator, separator, separator, separator);
    Serial.printf("Timestamp : %06u.%03u\r\n", now / 1000, now % 1000);
    if (results.overflow)
      Serial.printf("WARNING: IR code is too big for buffer (>= %d). "
                    "This result shouldn't be trusted until this is resolved. "
                    "Edit & increase CAPTURE_BUFFER_SIZE.\r\n",
                    CAPTURE_BUFFER_SIZE);
    // Output RAW timing info of the result.
//    Serial.println(resultToTimingInfo(&results));
//    yield();  // Feed the WDT

    // Output the results as source code
    Serial.println(resultToSourceCode(&results));
    Serial.print("IRCode hash: ");
    Serial.printf("%lX\r\n", (long)(results.value));
    yield();  // Feed the WDT (again)

    // Test purposes:
    // re-send the last code, but avoid a send-receive infinite loop
    // so call sendCode, after every other receive.
    if (receiveCounter & 1) {
      receiveAndReSendHash = (long)(results.value);
//      Serial.println(" will re-send code...");
//      delay(10);
//      sendCode(&results);
//      yield();
    } else { //Check also hash is the same, code re-sent is valid
        if (receiveAndReSendHash != (long)(results.value)) {
          Serial.print("Error: re-received hash ");
          Serial.printf("%lX", (long)(results.value));
          Serial.print(" doesn't match original ");
          Serial.printf("%lX", receiveAndReSendHash);
          Serial.println("   !!!");
          yield();
      }
    }
  }
}

void sendCode(const decode_results* results) {
//  if ((long)(results->value) < -1L) {
//    Serial.print("Code < 0: ");
//    Serial.printf("%lX", (long)(results->value));
//    Serial.println(" is invalid!");
//    return;
//  }
  uint16_t rawLength = getCorrectedRawLength(results);
  uint16_t rawData[rawLength];
  uint16_t idx = 0;
  // Dump data
  for (uint16_t i = 1; i < results->rawlen; i++) {
    uint32_t usecs;
    for (usecs = results->rawbuf[i] * RAWTICK;
         usecs > UINT16_MAX;
         usecs -= UINT16_MAX) {
      rawData[idx++] = UINT16_MAX;
    }
    rawData[idx++] = usecs;
  }

  if (rawLength != idx) {
    Serial.print("CorrectedRawLength=");
    Serial.print(rawLength);
    Serial.print(" iterated_length=");
    Serial.print(idx);
    Serial.println(" ERROR! Will not send code");
    return;
  }

  Serial.print("Sending IR code of ");
  Serial.print(rawLength);
  Serial.println(" length");

  // DEBUG:
  for (int i=0; i<rawLength; i++) {
    Serial.print(rawData[i]);
    Serial.print(" ");
  }
  Serial.println();

  irsend.sendRaw(rawData, rawLength, IR_SEND_KHz);
}

#ifdef TEST_RELIABILITY
// this will re-send the first received code
// over and over again, to test reliability of hash-code and data
long testCounter;
void testReliability() {
  testCounter ++;
  Serial.printf("Test reliability ### %d\r\n", testCounter);
  sendCode(&resultsTest);
}
#endif

bool setupFS(bool format) {
  Serial.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    Serial.println("FS error!");
    return false;
  }

  Serial.println("FS contents:");
  Dir dir = SPIFFS.openDir("/");
  if (!dir.next()) {
    Serial.println("\t<empty>");
  } else {
    do {
      Serial.print("\t");
      Serial.print(dir.fileName());
      Serial.print("\t");
      File f = dir.openFile("r");
      Serial.print(f.size());
      Serial.println(" Bytes");
      f.close();
    } while(dir.next());
  }
  //Format File System?
  if (format) {
    Serial.println("Formating filesystem...");
    long ts = millis();
    if(SPIFFS.format()) {
      Serial.print("File System Formated (took ");
      ts = millis() - ts;
      Serial.print(ts / 1000);
      Serial.print(",");
      Serial.print(ts % 1000);
      Serial.println("sec).");
    } else {
      Serial.println("File System Formatting Error");
    }
  }
  return true;
}
