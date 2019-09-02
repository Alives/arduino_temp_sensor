#include <ESP8266WiFi.h>

WiFiClient carbon_client;

void connectCarbonClient() {
  for (;;) {
    Serial.println(PSTR("Connecting to: ") + carbon_host);
    carbon_client.connect(carbon_host.c_str(), CARBON_PORT);
    client_connect_attempts += 1;
    if (carbon_client.connected()) {
      Serial.println(F("Carbon client connected."));
      return;
    }
    Serial.println(F("Carbon client connection failed!"));
    delay(5000);
  }
}

void handleCarbon() {
  if (!getWiFiStatus()) { return; }
  if (!carbon_client.connected()) { connectCarbonClient(); }

  updateEnvironment();
  // Read these prior to allocating data string.
  uint32_t free_heap = ESP.getFreeHeap();
  uint32_t max_free_block_size = ESP.getMaxFreeBlockSize();
  int heap_fragmentation_percent = ESP.getHeapFragmentation();

  char * payload = (char *) malloc(768 * sizeof(char));
  char * prefix = (char *) malloc((13 + strlen(sensor_name)) * sizeof(char));

  strcpy(prefix, PSTR("temp_sensor."));
  strcat(prefix, sensor_name);

  sprintf(payload, PSTR(
      "%s.client_connect_attempts %lu -1.\n"
      "%s.free_heap %lu -1.\n"
      "%s.heap_fragmentation_percent %d -1.\n"
      "%s.humidity_errors %lu -1.\n"
      "%s.humidity %0.2f -1.\n"
      "%s.max_free_block_size %lu -1.\n"
      "%s.RSSI %ld -1.\n"
      "%s.temperature_errors %lu -1.\n"
      "%s.tempC %0.2f -1.\n"
      "%s.tempF %0.2f -1.\n"
      "%s.wifi_connect_attempts %lu -1.\n"
      "%s.uptimeMS %lu -1.\n"),
      prefix, client_connect_attempts,
      prefix, free_heap,
      prefix, heap_fragmentation_percent,
      prefix, errors.humidity,
      prefix, env.h,
      prefix, max_free_block_size,
      prefix, WiFi.RSSI(),
      prefix, errors.temperature,
      prefix, env.c,
      prefix, env.f,
      prefix, wifi_connect_attempts,
      prefix, millis());
  free(prefix);
  carbon_client.write(payload);
  free(payload);
}
