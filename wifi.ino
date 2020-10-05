#define LED 2
#include <WiFiManager.h>

WiFiManager wifi_manager;

const char _customHtml_radio[] = "type=\"radio\"";
WiFiManagerParameter param_write_carbon("destination", "Write Carbon",
    "carbon", 7, _customHtml_radio, WFM_LABEL_BEFORE);

WiFiManagerParameter param_write_https("destination", "Write HTTPS",
    "https", 6, _customHtml_radio, WFM_LABEL_BEFORE);

WiFiManagerParameter param_carbon_host(
    "carbon_host",
    "Carbon Host",
    carbon_host.c_str(), 41);

WiFiManagerParameter param_https_host(
    "https_post_server",
    "HTTPS Post Server",
    https_host.c_str(), 41);

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
  WiFi.hostname(sensor_name);
  Serial.println(F("WiFi Reconnecting..."));
  Serial.println(PSTR("MAC address: ") + WiFi.macAddress());
  Serial.println(PSTR("Connecting to SSID: ") + WiFi.SSID());

  wifi_manager.setConfigPortalTimeout(60);
  wifi_manager.setSaveParamsCallback(saveParams);

  wifi_manager.addParameter(&param_write_carbon);
  wifi_manager.addParameter(&param_write_https);
  wifi_manager.addParameter(&param_carbon_host);
  wifi_manager.addParameter(&param_https_host);
  wifi_manager.addParameter(&param_ota_password);
  wifi_manager.addParameter(&param_metric_hostname);

  char * ssid = (char *) malloc((8 + strlen(sensor_name)) * sizeof(char));
  sprintf(ssid, PSTR("sensor_%s"), sensor_name);
  analogWrite(LED, 10);
  if ((destination.length() + carbon_host.length() + https_host.length() +
       ota_password.length() + metric_hostname.length()) == 0) {
    wifi_manager.startConfigPortal(ssid, NULL);
  }
  if (!wifi_manager.autoConnect(ssid)) {
    Serial.println(F("Failed connecting to WiFi and timed out. Rebooting..."));
    ESP.reset();
  }
  free(ssid);

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
  // Both write_carbon and write_https will have the same value.
  value = param_write_carbon.getValue();
  if (!destination.equals(value)) {
    destination = value;
    writeFile(PSTR("/destination"), destination);
  }

  value = param_carbon_host.getValue();
  if (!carbon_host.equals(value)) {
    carbon_host = value;
    writeFile(PSTR("/carbon_host"), carbon_host);
  }

  value = param_https_host.getValue();
  if (!https_host.equals(value)) {
    https_host = value;
    writeFile(PSTR("/https_host"), https_host);
  }

  value = param_ota_password.getValue();
  if (!ota_password.equals(value)) {
    ota_password = value;
    writeFile(PSTR("/ota_password"), ota_password);
  }

  value = param_metric_hostname.getValue();
  if (!metric_hostname.equals(value)) {
    metric_hostname = value;
    writeFile(PSTR("/metric_hostname"), metric_hostname);
  }
}
