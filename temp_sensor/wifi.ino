#include <WiFiManager.h>

WiFiManager wifiManager;

bool getWiFiStatus () {
  return (WiFi.status() == WL_CONNECTED);
}

void handleWiFi () {
  if (getWiFiStatus()) { return; }
  Serial.println(F("WiFi Reconnecting..."));
  Serial.println(PSTR("MAC address: ") + WiFi.macAddress());
  Serial.println(PSTR("Connecting to SSID: ") + WiFi.SSID());
  int onoff = 0;
  digitalWrite(2, 0);
  if (!wifiManager.autoConnect(ssid.c_str())) {
    Serial.println(F("Failed connecting to WiFi and timed out. Rebooting..."));
    ESP.reset();
  }

  while (!getWiFiStatus()) {
    digitalWrite(2, onoff);
    onoff ^= 1;
    delay(25);
  }
  digitalWrite(2, 1);
}

void setupWiFi () {
  ssid = sensor_name + ' ' + WiFi.macAddress();
  WiFi.hostname(sensor_name);
  wifiManager.setConfigPortalTimeout(30);
}
