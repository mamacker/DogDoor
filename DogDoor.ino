#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"
#define WIFI_SSID "*****"
#define WIFI_PASS "*****"
#define SERIAL_BAUDRATE                 115200
#define DEBUG_FAUXMO_VERBOSE true
#define ON_PIN 12
#define OPEN_PIN 13
fauxmoESP fauxmo;
int doorOpen;
#define REBOOT_TIME 345600000  // 1000 * 60 * 60 * 24 * 4 : 4 days

// Matt's Notes!!!  
// Requires ESP8266 Board Library version: 2.3.0
// Requires single-word device name.
// Requires 2.3.0
// These things are all outside the control of this code. (:

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------
 
void wifiSetup() {
  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);

  // Connect
  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
  }
  Serial.println();

  // Connected!
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}


void setup() {
  // put your setup code here, to run once:
  pinMode(ON_PIN, OUTPUT);
  pinMode(OPEN_PIN, OUTPUT);
  digitalWrite(ON_PIN, HIGH);
  digitalWrite(OPEN_PIN, HIGH);

  doorOpen = -1;
  // Init serial port and clean garbage
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();
  Serial.println();

  // Wifi
  wifiSetup();

  // Fauxmo v2.0
  //fauxmo.addDevice("dog door");  //Doesn't work due to space.
  fauxmo.addDevice("dog");
  fauxmo.enable(true);
  
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state) {
      Serial.printf("[MAIN] Device #%d (%s) state: %s\n", device_id, device_name, state ? "ON" : "OFF");
      if (state) {
        doorOpen = 1; 
      } else {
        doorOpen = 0;     
      }
  });
}

void forceClose() {
  digitalWrite(ON_PIN, HIGH);
  digitalWrite(OPEN_PIN, LOW);
  delay(300);
  digitalWrite(OPEN_PIN, HIGH);
  delay(300);
  digitalWrite(OPEN_PIN, LOW);
  delay(100);
  digitalWrite(OPEN_PIN, HIGH);
}

void loop() {
  fauxmo.handle();
  static int lastDoorState = doorOpen;
  if (doorOpen != lastDoorState) {
    if (lastDoorState == -1) {
      // Force the door to reset state.
      forceClose();

      if (doorOpen == 0) {
        lastDoorState = 0;
        return;
      } 
      delay(9000);
    }
    
    if (doorOpen == 1) {
      digitalWrite(ON_PIN, LOW);
      delay(100);
      digitalWrite(OPEN_PIN, LOW);
      delay(300);
      digitalWrite(ON_PIN, HIGH);
      digitalWrite(OPEN_PIN, HIGH);
    } else if (doorOpen == 0) {
      forceClose();
      if (millis() > REBOOT_TIME ) {
        ESP.restart();
      }
    }
    lastDoorState = doorOpen; 
  }
  
  static unsigned long last = millis();
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
  }
}
