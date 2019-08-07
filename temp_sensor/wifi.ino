#define LED 2
#include <WiFiManager.h>

bool getWiFiStatus() {
  return (WiFi.status() == WL_CONNECTED);
}

void handleWiFi() {
  digitalWrite(LED, HIGH);
  if (getWiFiStatus()) { return; }
  setupWiFi();
}

void setupWiFi() {
  WiFi.hostname(String(sensor_name));
  Serial.println(F("WiFi Reconnecting..."));
  Serial.println(PSTR("MAC address: ") + WiFi.macAddress());
  Serial.println(PSTR("Connecting to SSID: ") + WiFi.SSID());

  WiFiManager wifi_manager;
  wifi_manager.setConfigPortalTimeout(60);
  WiFiManagerParameter custom_https_post_server(
      "https_post_server", "HTTPS Post Server", host.c_str(), 41);
  WiFiManagerParameter arduino_ota_password(
      "ota_password", "OTA Password", ota_password.c_str(), 41);
  wifi_manager.addParameter(&custom_https_post_server);
  wifi_manager.addParameter(&arduino_ota_password);
  char * ssid = (char *) malloc(
      (1 + strlen(sensor_name) + WiFi.macAddress().length()) * sizeof(char));
  sprintf(ssid, PSTR("%s %s"), sensor_name, WiFi.macAddress().c_str());
  analogWrite(LED, 128);
  if (!wifi_manager.autoConnect(ssid)) {
    Serial.println(F("Failed connecting to WiFi and timed out. Rebooting..."));
    ESP.reset();
  }

  while (!getWiFiStatus()) {
    delay(30);
  }
  analogWrite(LED, 0);

  if (host.length() == 0) {
    host = String(custom_https_post_server.getValue());
    writeFile(PSTR("/host"), host);
  }
  if (ota_password.length() == 0) {
    ota_password = String(arduino_ota_password.getValue());
    writeFile(PSTR("/pass"), ota_password);
  }
  WiFi.softAPdisconnect(true);
}
