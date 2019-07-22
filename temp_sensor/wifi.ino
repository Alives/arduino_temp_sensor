#include <WiFiManager.h>

String ssid;
WiFiManager wifiManager;

String getRSSI () {
  return String(WiFi.RSSI());
}

bool getWiFiStatus () {
  return (WiFi.status() == WL_CONNECTED);
}

void handleWiFi () {
  if (getWiFiStatus()) { return; }

  syslog(F("WiFi Reconnecting..."));
  syslog(PSTR("MAC address: ") + WiFi.macAddress());
  syslog(PSTR("Connecting to SSID: ") + WiFi.SSID());

  int onoff = 0;

  digitalWrite(2, 0);

  if (!wifiManager.autoConnect(ssid.c_str())) {
    syslog(F("Failed to connect to WiFi and hit timeout. Rebooting..."));
    ESP.reset();
  }

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, onoff);
    onoff ^= 1;
    delay(25);
  }
  digitalWrite(2, 1);

  syslog(F("Connected!"));
  syslog(PSTR("RSSI: ") + getRSSI() + PSTR(" dBm"));
  syslog(PSTR("IP address: ") + WiFi.localIP().toString());
  syslog(PSTR("Subnet mask: ") + WiFi.subnetMask().toString());
  syslog(PSTR("Gateway: ") + WiFi.gatewayIP().toString());
}

void setupWiFi () {
  WiFi.hostname(getSensorName());
  wifiManager.setConfigPortalTimeout(30);
  ssid = getSensorName() + ' ' + WiFi.macAddress();
}
