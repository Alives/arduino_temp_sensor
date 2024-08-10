#include <Ethernet.h>
#include <ESP8266WiFi.h>

#define VERSION "11.0"
#define CARBON_PORT 2003
#define HTTPS_PORT 443
#define POST_INTERVAL 5000

#define VERSION_ArduinoOTA "1.1.0"
#define VERSION_Adafruit_BME280 "2.2.4"
#define VERSION_Adafruit_Unified_Sensor "1.1.14"
#define VERSION_esp8266_firmware "3.1.2"
#define VERSION_WiFiManager "2.0.17"


struct env_t {
  float altitude,
        celcius,
        dewpointC,
        dewpointF,
        fahrenheit,
        humidity,
        pressure;
} env;
uint32_t client_connect_attempts = 0L;
uint32_t next_post_timestamp = 0L;
uint32_t wifi_connect_attempts = 0L;
String carbon_host;
String ota_password;
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
  Serial.begin(115200);
  Serial.println("\rDelaying for serial monitor...");
  delay(POST_INTERVAL);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);

  Serial.println("\r\n===========================================");
  Serial.print("Arduino Core:            ");
  Serial.println(__VERSION__);
  Serial.print("ESP8266 Core:            ");
  Serial.println(ESP.getCoreVersion());
  Serial.print("SDK:                     ");
  Serial.println(ESP.getSdkVersion());
  Serial.print("esp8266 Board Firmware:  ");
  Serial.println(VERSION_esp8266_firmware);
  Serial.println();
  Serial.print("Adafruit BME:            ");
  Serial.println(VERSION_Adafruit_BME280);
  Serial.print("Adafruit Unified Sensor: ");
  Serial.println(VERSION_Adafruit_Unified_Sensor);
  Serial.print("ArduinoOTA:              ");
  Serial.println(VERSION_ArduinoOTA);
  Serial.print("WiFiManager:             ");
  Serial.println(VERSION_WiFiManager);
  Serial.println();
  Serial.print("Code Version:            ");
  Serial.println(VERSION);
  setSensorName();
  Serial.print("Chip ID:                 0x");
  Serial.println(String(ESP.getChipId(), HEX) + " (" + sensor_name + ")");
  Serial.println("===========================================\n");

  setupFS();
  carbon_host = readFile("/carbon_host");
  ota_password = readFile("/ota_password");
  sensor_name = readFile("/sensor_name");
  Serial.println();

  connectWiFi();
  Serial.println("\r\nWiFi started.");
  setupArduinoOTA();
  Serial.println("ArduinoOTA started.");
  setupBME();
  Serial.println("BME280 started.");
  Serial.println("Entering run loop...");
}

void loop() {
  handleWiFi();
  handleArduinoOTA();
  handleHTTPServer();
  if (millis() >= next_post_timestamp) {
    next_post_timestamp = millis() + POST_INTERVAL;
    if (sensor_name.length() == 0) {
      Serial.print("Not sending metrics, setup needed: ");
      Serial.println(WiFi.localIP());
    } else {
      handleCarbon();
    }
  }
  yield();
}
