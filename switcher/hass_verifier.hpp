

HTTPClient http;
WiFiClient httpClientRequest;


bool verifyHass(void *) {
  String formattedTime = timeClient.getFormattedTime();
  Serial.print("Verifying Hass on: ");
  Serial.println(formattedTime);  

  http.begin(httpClientRequest, "http://192.168.1.27:8123/");
  http.addHeader("Accept", "*/*");
  http.addHeader("Accept-Encoding", "gzip, deflate");
  http.addHeader("User-Agent", "HTTPie/2.4.0");
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  

  http.end();

  return true; // repeat? true
}
