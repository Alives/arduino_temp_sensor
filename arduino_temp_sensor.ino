#include <Ethernet.h>
#include <ESP8266WiFi.h>

#define VERSION "9.0"
#define CARBON_PORT 2003
#define HTTPS_PORT 443
#define POST_INTERVAL 5000

struct env_t { float altitude, celcius, fahrenheit, humidity, pressure; } env;
uint32_t client_connect_attempts = 0L;
uint32_t next_post_timestamp = 0L;
uint32_t wifi_connect_attempts = 0L;
String carbon_host;
String ota_password;
String metric_hostname;
String prefix;
String sensor_name;

void setSensorName () {
  struct sensor_s {
    const int sensor_id;
    const char * sensor_name;
  };
  sensor_s sensors[] = {
    {0x92ea79, "office"},
    {0xd65b01, "livingroom"},
    {0xd658f7, "kid"},
    {0x068231, "bedroom"},
    {0x085762, "outside"},
    {0xc6e1ff, "_huzzah"},
    {0xd65878, "_outside"}
  };
  int id = ESP.getChipId();

  sensor_name = "Error";
  for (unsigned int i = 0; i < sizeof(sensors); i++) {
    if (sensors[i].sensor_id == id) {
      sensor_name = sensors[i].sensor_name;
      break;
    }
  }
}

void setup() {
  Serial.begin(19200);
  Serial.println("Delaying for serial monitor...");
  delay(POST_INTERVAL);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);

  setupFS();
  carbon_host = readFile("/carbon_host");
  ota_password = readFile("/ota_password");
  metric_hostname = readFile("/metric_hostname");

  Serial.print("Version ");
  Serial.println(VERSION);
  setSensorName();
  Serial.print("Chip ID: 0x");
  Serial.println(String(ESP.getChipId(), HEX) + " (" + sensor_name + ")");

  setupWiFi();
  Serial.println("WiFi started.");
  setupArduinoOTA();
  Serial.println("ArduinoOTA started.");
  setupBME();
  Serial.println("BME280 started.");
  Serial.println("\nEntering run loop...");
}

void loop() {
  handleWiFi();
  handleArduinoOTA();
  handleHTTPServer();
  if (millis() >= next_post_timestamp) {
    next_post_timestamp = millis() + POST_INTERVAL;
    if (metric_hostname.length() == 0) {
      Serial.print("Not sending metrics, setup needed: ");
      Serial.println(WiFi.localIP());
    } else {
      handleCarbon();
    }
  }
  yield();
}
