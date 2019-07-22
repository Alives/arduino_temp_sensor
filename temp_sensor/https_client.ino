#include <WiFiClientSecure.h>

WiFiClientSecure https_client;
String host;
const int https_port = 443;
uint32_t https_connect_attempts = 0L;
uint32_t next_post_timestamp = 0L;

void connectHTTPSClient() {
  for (;;) {
    https_client.setInsecure();
    https_client.connect(host.c_str(), https_port);
    https_connect_attempts += 1;
    if (https_client.connected()) {
      syslog(F("HTTPS client connected."));
      return;
    }
    syslog(F("HTTPS client connection failed!"));
    delay(5000);
  }
}

String getHTTPSConnectAttepts() {
  return String(https_connect_attempts);
}

String getSensorData() {
  env_t env = getEnvironment();
  return String(
    PSTR("{\"free_heap\": ") +
    String(ESP.getFreeHeap()) +
    PSTR(", \"heap_fragmentation_percent\": ") +
    String(ESP.getHeapFragmentation()) +
    PSTR(", \"https_connect_attempts\": ") +
    String(https_connect_attempts) +
    PSTR(", \"humidity_errors\": ") +
    String(errors.humidity) +
    PSTR(", \"humidity\": ") +
    String(env.h) +
    PSTR(", \"max_free_block_size\": ") +
    String(ESP.getMaxFreeBlockSize()) +
    PSTR(", \"RSSI\": ") +
    getRSSI() +
    PSTR(", \"temperature_errors\": ") +
    errors.temperature +
    PSTR(", \"tempC\": ") +
    String(env.c) +
    PSTR(", \"tempF\": ") +
    String(env.f) +
    PSTR(", \"uptimeMS\": ") +
    String(millis()) +
    PSTR("}"));
}

String getPostData() {
  return String(
    PSTR("<prefix>temp_sensor.") + getSensorName() + PSTR("</prefix>") +
    PSTR("<data>") + getSensorData() + PSTR("</data>") +
    PSTR("<log>") + getSyslog() + PSTR("</log>"));
}


void postData () {
  if (!getWiFiStatus()) { return; }
  if (!https_client.connected()) { connectHTTPSClient(); }

  String data = getPostData();

  https_client.print(String(
    PSTR("POST / HTTP/1.1\r\n")) +
    PSTR("Host: ") + host + PSTR("\r\n") +
    PSTR("User-Agent: ") + getSensorName() + PSTR("\r\n") +
    PSTR("Content-Length: ") + String(data.length()) + PSTR("\r\n\r\n") +
    data);
  clearSyslog();

  if (https_client.available()) {
    String status = https_client.readStringUntil('\r');
    https_client.readString();  // flush the buffer.
    if (!status.startsWith(PSTR("HTTP/1.1 200"))) {
      syslog(status);
    }
  }
}

void setupHTTPSClient() {
  host = PSTR(HOST);
}

void handleHTTPSClient () {
  if (millis() >= next_post_timestamp) {
    next_post_timestamp = millis() + HTTP_POST_INTERVAL;
    postData();
  }
}
