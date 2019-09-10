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

void onJSONCallback() {
  wifi_manager.server->on(PSTR("/json"), sendJSON);
}

void sendJSON() {
  updateEnvironment();
  // Read these prior to allocating data string.
  uint32_t free_heap = ESP.getFreeHeap();
  uint32_t max_free_block_size = ESP.getMaxFreeBlockSize();
  int heap_fragmentation_percent = ESP.getHeapFragmentation();

  char * response = (char *) malloc(384 * sizeof(char));
  sprintf(response, PSTR(
      "{\n"
        "\"Chip ID\":\"0x%X (%s)\",\n"
        "\"Version\":\"%s\",\n"
        "\"client_connect_attempts\":%lu,\n"
        "\"free_heap\":%lu,\n"
        "\"heap_fragmentation_percent\":%d,\n"
        "\"humidity_errors\":%lu,\n"
        "\"humidity\":%0.2f,\n"
        "\"max_free_block_size\":%lu,\n"
        "\"RSSI\":%ld,\n"
        "\"sensor_name\":%s,\n"
        "\"temperature_errors\":%lu,\n"
        "\"tempC\":%0.2f,\n"
        "\"tempF\":%0.2f,\n"
        "\"wifi_connect_attempts\":%lu,\n"
        "\"uptimeMS\":%lu\n"
      "}"),
      ESP.getChipId(),
      sensor_name,
      PSTR(VERSION),
      client_connect_attempts,
      free_heap,
      heap_fragmentation_percent,
      errors.humidity,
      env.h,
      max_free_block_size,
      WiFi.RSSI(),
      sensor_name,
      errors.temperature,
      env.c,
      env.f,
      wifi_connect_attempts,
      millis());
  wifi_manager.server->send(200, F("text/plain"), response);
  free(response);
}

void setupWiFi() {
  WiFi.hostname(String(sensor_name));
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

  char * ssid = (char *) malloc((8 + strlen(sensor_name)) * sizeof(char));
  sprintf(ssid, PSTR("sensor_%s"), sensor_name);
  analogWrite(LED, 10);
  if ((destination.length() == 0) ||
      (carbon_host.length() == 0) ||
      (https_host.length() == 0) ||
      (ota_password.length() == 0)) {
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
  wifi_manager.setWebServerCallback(onJSONCallback);
  wifi_manager.startWebPortal();
  WiFi.mode(WIFI_STA);
}

void saveParams() {
  String value;
  // Both write_carbon and write_https will have the same value.
  value = param_write_carbon.getValue();
  if ((value.length() > 0) && (!destination.equals(value))) {
    destination = value;
    writeFile(PSTR("/destination"), destination);
  }

  value = param_carbon_host.getValue();
  if ((value.length() > 0) && (!carbon_host.equals(value))) {
    carbon_host = value;
    writeFile(PSTR("/carbon_host"), carbon_host);
  }

  value = param_https_host.getValue();
  if ((value.length() > 0) && (!https_host.equals(value))) {
    https_host = value;
    writeFile(PSTR("/https_host"), https_host);
  }

  value = param_ota_password.getValue();
  if ((value.length() > 0) && (!ota_password.equals(value))) {
    ota_password = value;
    writeFile(PSTR("/ota_password"), ota_password);
  }
}
