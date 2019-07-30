#include <WiFiClientSecure.h>

WiFiClientSecure https_client;

void connectHTTPSClient() {
  for (;;) {
    https_client.setInsecure();
    https_client.connect(host, HTTPS_PORT);
    https_connect_attempts += 1;
    if (https_client.connected()) {
      Serial.println(F("HTTPS client connected."));
      return;
    }
    Serial.println(F("HTTPS client connection failed!"));
    delay(5000);
  }
}

void postData () {
  if (!getWiFiStatus()) { return; }
  if (!https_client.connected()) { connectHTTPSClient(); }

  updateEnvironment();

  // Read these prior to allocating data string.
  uint32_t free_heap = ESP.getFreeHeap();
  uint32_t max_free_block_size = ESP.getMaxFreeBlockSize();
  int heap_fragmentation_percent = ESP.getHeapFragmentation();

  char * data = (char *) malloc(128 * sizeof(char));

  sprintf(data, PSTR("%lu,%d,%lu,%lu,%0.2f,%lu,%ld,%s,%lu,%0.2f,%0.2f,%lu,%s"),
    free_heap,
    heap_fragmentation_percent,
    https_connect_attempts,
    errors.humidity,
    env.h,
    max_free_block_size,
    WiFi.RSSI(),
    sensor_name.c_str(),
    errors.temperature,
    env.c,
    env.f,
    millis(),
    PSTR(VERSION));
  data = (char *) realloc(data, (strlen(data) + 1) * sizeof(char));

  char * post = (char *) malloc(
      (strlen(data) + host.length() + sensor_name.length() + 70)
      * sizeof(char));
  sprintf(post, PSTR(
    "POST / HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: %s\r\n"
    "Content-Length: %lu\r\n\r\n"
    "%s"),
    host.c_str(),
    sensor_name.c_str(),
    strlen(data),
    data);
  free(data);
  https_client.write(post);
  free(post);

  if (https_client.available()) {
    String status = https_client.readStringUntil('\r');
    https_client.readString();  // flush the buffer.
    if (!status.startsWith(PSTR("HTTP/1.1 200"))) {
      Serial.println(status);
    }
  }
}

void handleHTTPSClient () {
  if (millis() >= next_post_timestamp) {
    next_post_timestamp = millis() + HTTP_POST_INTERVAL;
    postData();
  }
}
