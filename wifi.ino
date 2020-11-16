#define LED 2
#include <WiFiManager.h>

WiFiManager wifi_manager;

WiFiManagerParameter param_carbon_host(
    "carbon_host",
    "Carbon Host",
    carbon_host.c_str(), 41);

WiFiManagerParameter param_ota_password(
    "ota_password",
    "OTA Password",
    ota_password.c_str(), 41);

WiFiManagerParameter param_metric_hostname(
    "metric_hostname",
    "Metric Hostname",
    metric_hostname.c_str(), 41);

bool getWiFiStatus() {
  return (WiFi.status() == WL_CONNECTED);
}

void handleWiFi() {
  digitalWrite(LED, HIGH);
  if (getWiFiStatus()) { return; }
  setupWiFi();
}

void handleHTTPServer() {
  wifi_manager.process();
}

void setupWiFi() {
  String ssid = "sensor_" + sensor_name;
  WiFi.hostname(sensor_name);
  Serial.println("WiFi Reconnecting...");
  Serial.println("MAC address: " + WiFi.macAddress());
  Serial.println("Connecting to SSID: " + WiFi.SSID());

  wifi_manager.setConfigPortalTimeout(60);
  wifi_manager.setSaveParamsCallback(saveParams);

  wifi_manager.addParameter(&param_carbon_host);
  wifi_manager.addParameter(&param_ota_password);
  wifi_manager.addParameter(&param_metric_hostname);

  analogWrite(LED, 10);
  if ((carbon_host.length() + ota_password.length() + metric_hostname.length()) == 0) {
    wifi_manager.startConfigPortal(ssid.c_str(), NULL);
  }
  if (!wifi_manager.autoConnect(ssid.c_str())) {
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

  value = param_metric_hostname.getValue();
  if (!metric_hostname.equals(value)) {
    metric_hostname = value;
    writeFile("/metric_hostname", metric_hostname);
  }
}
