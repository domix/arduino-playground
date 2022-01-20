#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <arduino-timer.h>

#include "wifi_creds.h"
#include "ESP8266_Utils.hpp"

#include "index.h"
#include "Server.hpp"
#include "set_time.hpp"
#include "async_task.hpp"
#include "hass_verifier.hpp"

auto timer = timer_create_default();

void setup() {
  Serial.begin(115200);

  ConnectWiFi_STA();
  initServer();

  timeClient.begin();
  timeClient.update();

  // call the toggle_led function every minute
  timer.every(60000, toggle_led);
  timer.every(60000, verifyHass);

  
}

void loop() {
  server.handleClient();
  timer.tick(); // tick the timer
}
