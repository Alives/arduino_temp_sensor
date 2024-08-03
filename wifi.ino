#define LED 2
#include <WiFiManager.h>

WiFiManager wifi_manager;


String version_info = String("") +
    "<table border='1' cellpadding='3' cellspacing='0'>" +
    "<tr><th>Component</th><th>Version</th></tr>" +
    "<tr><td>Arduino Core</td><td>" + String(__VERSION__) + "</td></tr>" +
    "<tr><td>ESP8266 Core</td><td>" + ESP.getCoreVersion() + "</td></tr>" +
    "<tr><td>SDK</td><td>" + String(ESP.getSdkVersion()) + "</td></tr>" +
    "<tr><td>esp8266 Board Firmware</td><td>" + VERSION_esp8266_firmware +
        "</td></tr>" +
    "<tr><td>Adafruit BME</td><td>" + VERSION_Adafruit_BME280 + "</td></tr>" +
    "<tr><td>Adafruit Unified Sensor</td><td>" + VERSION_Adafruit_Unified_Sensor +
        "</td></tr>" +
    "<tr><td>ArduinoOTA</td><td>" + VERSION_ArduinoOTA + "</td></tr>" +
    "<tr><td>WiFiManager</td><td>" + VERSION_WiFiManager + "</td></tr>" +
    "<tr><td>Code Version</td><td>" + VERSION + "</td></tr>" +
    "</table><br/>";
WiFiManagerParameter param_version_info(version_info.c_str());

WiFiManagerParameter param_carbon_host(
    "carbon_host",
    "Carbon Host",
    carbon_host.c_str(), 41);

WiFiManagerParameter param_ota_password(
    "ota_password",
    "OTA Password",
    ota_password.c_str(), 41);

WiFiManagerParameter param_sensor_name(
    "sensor_name",
    "Sensor Name",
    sensor_name.c_str(), 41);


void connectWiFi() {
  Serial.println("WiFi Connecting...");
  setupWiFi();
}

bool getWiFiStatus() {
  return (WiFi.status() == WL_CONNECTED);
}

void handleWiFi() {
  digitalWrite(LED, HIGH);
  if (getWiFiStatus()) { return; }
  Serial.println("WiFi Reconnecting...");
  setupWiFi();
}

void handleHTTPServer() {
  wifi_manager.process();
}

void setupWiFi() {
  String sensor_hostname = String("sensor_") + sensor_name;
  WiFi.hostname(sensor_hostname);
  Serial.println("MAC address: " + WiFi.macAddress());
  Serial.println("Connecting to SSID: " + WiFi.SSID());

  wifi_manager.setConfigPortalTimeout(60);
  wifi_manager.setSaveParamsCallback(saveParams);

  wifi_manager.addParameter(&param_version_info);
  wifi_manager.addParameter(&param_carbon_host);
  wifi_manager.addParameter(&param_ota_password);
  wifi_manager.addParameter(&param_sensor_name);

  analogWrite(LED, 10);
  if ((carbon_host.length() + ota_password.length() + sensor_name.length()) == 0) {
    wifi_manager.startConfigPortal(sensor_hostname.c_str(), NULL);
  }
  if (!wifi_manager.autoConnect(sensor_hostname.c_str())) {
    Serial.println("Failed connecting to WiFi and timed out. Rebooting...");
    ESP.reset();
  }

  while (!getWiFiStatus()) {
    delay(30);
  }
  analogWrite(LED, 0);
  wifi_manager.stopConfigPortal();
  wifi_manager.setCaptivePortalEnable(false);
  wifi_manager.startWebPortal();
  WiFi.mode(WIFI_STA);
  Serial.print("Hostname: ");
  Serial.println(WiFi.hostname());
  Serial.print(String("Primary DNS Server: "));
  Serial.println(WiFi.dnsIP(0));
  Serial.print(String("Secondary DNS Server: "));
  Serial.println(WiFi.dnsIP(1));
  Serial.print(String("Gateway: "));
  Serial.println(WiFi.gatewayIP());
  Serial.print(String("Subnet Mask: "));
  Serial.println(WiFi.subnetMask());
}

void saveParams() {
  String value;

  value = param_carbon_host.getValue();
  if (!carbon_host.equals(value)) {
    carbon_host = value;
    writeFile("/carbon_host", carbon_host);
  }

  value = param_ota_password.getValue();
  if (!ota_password.equals(value)) {
    ota_password = value;
    writeFile("/ota_password", ota_password);
  }

  value = param_sensor_name.getValue();
  if (!sensor_name.equals(value)) {
    sensor_name = value;
    writeFile("/sensor_name", sensor_name);
  }
}
