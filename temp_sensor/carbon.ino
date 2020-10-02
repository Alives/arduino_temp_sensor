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
  uint32_t free_heap = ESP.getFreeHeap();
  uint32_t max_free_block_size = ESP.getMaxFreeBlockSize();
  int heap_fragmentation_percent = ESP.getHeapFragmentation();

  char * prefix = (char *) malloc(
      (13 + metric_hostname.length()) * sizeof(char));
  strcpy(prefix, PSTR("temp_sensor."));
  strcat(prefix, metric_hostname.c_str());
  Serial.println(F("Sending carbon data:"));

  // 32 chars for all numeric conversions for simplicity.
  // 7 chars for '.  -1.\n'
  // 32 chars for metric name (heap_fragmentation_percent is only 26)
  char * payload = (char *) malloc(
      (strlen(prefix) + 32 + 7 + 32) * sizeof(char));

  sprintf(payload, PSTR("%s.altitude %0.2f -1.\n"), prefix, env.altitude);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.celcius %0.2f -1.\n"), prefix, env.celcius);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.fahrenheit %0.2f -1.\n"), prefix, env.fahrenheit);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.humidity %0.2f -1.\n"), prefix, env.humidity);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.pressure %0.2f -1.\n"), prefix, env.pressure);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.client_connect_attempts %lu -1.\n"), prefix,
          client_connect_attempts);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.free_heap %lu -1.\n"), prefix, free_heap);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.heap_fragmentation_percent %d -1.\n"), prefix,
          heap_fragmentation_percent);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.max_free_block_size %lu -1.\n"), prefix,
          max_free_block_size);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.RSSI %ld -1.\n"), prefix, WiFi.RSSI());
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.wifi_connect_attempts %lu -1.\n"), prefix,
          wifi_connect_attempts);
  Serial.println(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.uptimeMS %lu -1.\n"), prefix, millis());
  Serial.println(payload);
  carbon_client.write(payload);

  free(payload);
  free(prefix);
}
