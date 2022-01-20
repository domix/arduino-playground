ESP8266WebServer server(80);

void handleRoot() {
  String response = "ON";

  server.setContentLength(sizeof(HTML_PART_1) + sizeof(response) + sizeof(HTML_PART_2));
  server.send(200, "text/html", HTML_PART_1);
  server.sendContent(response);
  server.sendContent(HTML_PART_2);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void initServer() {
   server.on("/", handleRoot);
   server.onNotFound(handleNotFound);
   server.begin();
   Serial.println("HTTP server started");
}
