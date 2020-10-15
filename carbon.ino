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
    delay(POST_INTERVAL);
  }
}

void handleCarbon() {
  if (!getWiFiStatus()) { return; }
  if (!carbon_client.connected()) { connectCarbonClient(); }

  updateEnvironment();
  uint32_t free_heap = ESP.getFreeHeap();
  uint32_t max_free_block_size = ESP.getMaxFreeBlockSize();
  int heap_fragmentation_percent = ESP.getHeapFragmentation();

  String prefix = PSTR("temp_sensor.") + metric_hostname;
  Serial.println(PSTR("\nSending carbon data to ") + carbon_host +
                 PSTR(":") + CARBON_PORT + PSTR("/udp:"));

  // 32 chars for all numeric conversions for simplicity.
  // 7 chars for '.  -1.\n'
  // 32 chars for metric name (heap_fragmentation_percent is only 26)
  char * payload = (char *) malloc(
      (prefix.length() + 32 + 7 + 32) * sizeof(char));

  sprintf(payload, PSTR("%s.altitude %0.2f -1.\n"), prefix.c_str(),
          env.altitude);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.celcius %0.2f -1.\n"), prefix.c_str(), env.celcius);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.fahrenheit %0.2f -1.\n"), prefix.c_str(),
          env.fahrenheit);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.humidity %0.2f -1.\n"), prefix.c_str(),
          env.humidity);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.pressure %0.2f -1.\n"), prefix.c_str(),
          env.pressure);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.client_connect_attempts %lu -1.\n"), prefix.c_str(),
          client_connect_attempts);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.free_heap %lu -1.\n"), prefix.c_str(), free_heap);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.heap_fragmentation_percent %d -1.\n"),
          prefix.c_str(), heap_fragmentation_percent);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.max_free_block_size %lu -1.\n"), prefix.c_str(),
          max_free_block_size);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.RSSI %ld -1.\n"), prefix.c_str(), WiFi.RSSI());
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.wifi_connect_attempts %lu -1.\n"), prefix.c_str(),
          wifi_connect_attempts);
  Serial.print(payload);
  carbon_client.write(payload);

  sprintf(payload, PSTR("%s.uptimeMS %lu -1.\n"), prefix.c_str(), millis());
  Serial.print(payload);
  carbon_client.write(payload);

  free(payload);
}
