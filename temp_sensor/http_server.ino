#include <ESP8266WebServer.h>

ESP8266WebServer http_server(80);

void httpRoot () {
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
      "\"free_heap\":%lu,\n"
      "\"heap_fragmentation_percent\":%d,\n"
      "\"https_connect_attempts\":%lu,\n"
      "\"humidity_errors\":%lu,\n"
      "\"humidity\":%0.2f,\n"
      "\"max_free_block_size\":%lu,\n"
      "\"RSSI\":%ld,\n"
      "\"sensor_name\":%s,\n"
      "\"temperature_errors\":%lu,\n"
      "\"tempC\":%0.2f,\n"
      "\"tempF\":%0.2f,\n"
      "\"uptimeMS\":%lu\n"
    "}"),
    ESP.getChipId(),
    sensor_name.c_str(),
    PSTR(VERSION),
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
    millis());
  http_server.send(200, F("text/plain"), response);
  free(response);
}

void httpNoOp() { return; }  // Don't do anything.

void setupHTTPServer () {
  http_server.on(F("/favicon.ico"), httpNoOp);
  http_server.onNotFound(httpRoot);
  http_server.begin();
}

void handleHTTPServer () {
  http_server.handleClient();
}
