
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const long utcOffsetInSeconds = 3600 * -5;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

