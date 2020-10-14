#include <Ethernet.h>
#include <ESP8266WiFi.h>

#define VERSION "9.0"
#define CARBON_PORT 2003
#define HTTPS_PORT 443
#define POST_INTERVAL 5000

char * sensor_name;
struct env_t { float altitude, celcius, fahrenheit, humidity, pressure; } env;
uint32_t client_connect_attempts = 0L;
uint32_t next_post_timestamp = 0L;
uint32_t wifi_connect_attempts = 0L;
String carbon_host;
String destination;
String https_host;
String ota_password;
String metric_hostname;

void setSensorName () {
  struct sensor_s {
    const int sensor_id;
    const char * sensor_name;
  };
  sensor_s sensors[] = {
    {0x92ea79, PSTR("office")},
    {0xd65b01, PSTR("livingroom")},
    {0xd658f7, PSTR("kid")},
    {0x068231, PSTR("bedroom")},
    {0x085762, PSTR("outside")},
    {0xc6e1ff, PSTR("_huzzah")},
    {0xd65878, PSTR("_outside")}
  };
  int id = ESP.getChipId();

  sensor_name = (char *) malloc(16 * sizeof(char));
  strcpy(sensor_name, PSTR("ERROR"));
  for (unsigned int i = 0; i < sizeof(sensors); i++) {
    if (sensors[i].sensor_id == id) {
      sensor_name = (char *) realloc(sensor_name,
          (strlen(sensors[i].sensor_name) + 1) * sizeof(char));
      strcpy(sensor_name, sensors[i].sensor_name);
      break;
    }
  }
}

void setup() {
  Serial.begin(19200);
  Serial.println(PSTR("Delaying for serial monitor..."));
  delay(POST_INTERVAL);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);

  setupFS();
  destination = readFile(PSTR("/destination"));
  carbon_host = readFile(PSTR("/carbon_host"));
  https_host = readFile(PSTR("/https_host"));
  ota_password = readFile(PSTR("/ota_password"));
  metric_hostname = readFile(PSTR("/metric_hostname"));

  Serial.print(F("Version "));
  Serial.println(F(VERSION));
  setSensorName();
  Serial.print(F("Chip ID: 0x"));
  Serial.println(String(ESP.getChipId(), HEX) + F(" (") + sensor_name + ')');

  setupWiFi();
  Serial.println(F("WiFi started."));
  setupArduinoOTA();
  Serial.println(F("ArduinoOTA started."));
  setupBME();
  Serial.println(F("BME280 started."));
  Serial.println(F("\nEntering run loop..."));
}

void loop() {
  handleWiFi();
  handleArduinoOTA();
  handleHTTPServer();
  if (millis() >= next_post_timestamp) {
    next_post_timestamp = millis() + POST_INTERVAL;
    if (metric_hostname.length() == 0) {
      Serial.print(F("Not sending metrics, setup needed: "));
      Serial.println(WiFi.localIP());
    } else {
      if (destination == String(PSTR("carbon"))) {
        handleCarbon();
      } else if (destination == String(PSTR("https"))) {
        handleHTTPSClient();
      }
    }
  }
  yield();
}
