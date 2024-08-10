#include <ESP8266WiFi.h>

WiFiClient carbon_client;

void connectCarbonClient() {
  for (;;) {
    Serial.println(String("Connecting to carbon host ") +
                   carbon_host + String(":") + CARBON_PORT);
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

void send(String payload) {
  Serial.print(payload);
  carbon_client.write(payload.c_str());
}

void write_carbon(String metric, String value) {
  Serial.print(String("Sending "));

  send(String("temp_sensor."));
  send(sensor_name);
  send(String("."));
  send(metric);
  send(String(" "));
  send(value);
  send(String(" -1.\n"));
  Serial.print(String("\r"));
}

void handleCarbon() {
  if (!getWiFiStatus()) { return; }
  if (!carbon_client.connected()) { connectCarbonClient(); }

  updateEnvironment();

  write_carbon(String("altitude "), String(env.altitude));
  write_carbon(String("celcius "), String(env.celcius));
  write_carbon(String("dewpointC "), String(env.dewpointC));
  write_carbon(String("dewpointF "), String(env.dewpointF));
  write_carbon(String("fahrenheit "), String(env.fahrenheit));
  write_carbon(String("humidity "), String(env.humidity));
  write_carbon(String("pressure "), String(env.pressure));
  write_carbon(String("free_heap "), String(ESP.getFreeHeap()));
  write_carbon(String("heap_fragmentation_percent "),
                    String(ESP.getHeapFragmentation()));
  write_carbon(String("max_free_block_size "),
                    String(ESP.getMaxFreeBlockSize()));
  write_carbon(String("RSSI "), String(WiFi.RSSI()));
  write_carbon(String("wifi_connect_attempts "),
                    String(wifi_connect_attempts));
  write_carbon(String("uptimeMS "), String(millis()));
}
