/*
8.0:
 - Add option to write directly to carbon instead of via https.
 - Note: Remove arduino builtin libraries that conflict with esp8266 ones.
 - Switch to WiFiManager's HTTP server to handle config params and info pages.

 7.0:
 - Switched to WiFiManager development branch at commit 34d9a97.
 - Save host and ota_password to SPIFFS using WiFiManager to set them.
 - Added http update server: curl -F "image=@img.bin" fqdn/update
 - Stop wifi portal on successful connection.
 - Start wifiManager prior to HTTPServer.

6.0:
 - data now in concise format instead of json for less bandwidth use.
 - No more syslog.

5.0:
 - No 404, all URIs result in metrics output.
 - struct for temps and errors.
 - split all functions into specific files.
 - Use F()/PSTR() macros everywhere.
 - No syslog for OTA since it only outputs to Serial.

4.0:
 - Fixed issue where syslog was sent twice before clearing.
 - Also push log via HTTPS.
 - Clean up includes.
 - POST with sections designated by <> tags and data in json format.
 - Add heap and RSSI metrics.
 - Convert strings to F() where possible.

3.0:
 - Push data via https.
 - Use millis() for intervals and handle rollovers.
 - Added /reset function (though it probably doesn't work well).
 - Add more metrics to json output.
 - Return non-string json values as their own types (float/int).
 - Changed post interval to >= 5 seconds.

2.0:
 - Added WifiManager
*/

#include <Ethernet.h>

#define VERSION "8.0"
#define CARBON_PORT 2003
#define HTTPS_PORT 443
#define POST_INTERVAL 5000

char * sensor_name;
struct env_t { float c, f, h; } env;
struct sensor_errors_t { uint8_t humidity, temperature; } errors;
uint32_t client_connect_attempts = 0L;
uint32_t next_post_timestamp = 0L;
uint32_t wifi_connect_attempts = 0L;
String carbon_host;
String destination;
String https_host;
String ota_password;

void setSensorName () {
  struct sensor_s {
    const int sensor_id;
    const char * sensor_name;
  };
  sensor_s sensors[] = {
    {0xc6e1ff, PSTR("huzzah")},
    {0x92ea79, PSTR("den")},
    {0xd65b01, PSTR("kitchen")},
    {0xd658f7, PSTR("kid")},
    {0xd65878, PSTR("outside")},
    {0x068231, PSTR("bedroom")},
  };
  int id = ESP.getChipId();

  sensor_name = (char *) malloc(8 * sizeof(char));
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
  Serial.println();
  delay(100);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);

  errors = {0,0};
  setupFS();
  destination = readFile(PSTR("/destination"));
  carbon_host = readFile(PSTR("/carbon_host"));
  https_host = readFile(PSTR("/https_host"));
  ota_password = readFile(PSTR("/ota_password"));

  Serial.print(F("Version "));
  Serial.println(F(VERSION));
  setSensorName();
  Serial.print(F("Chip ID: 0x"));
  Serial.println(String(ESP.getChipId(), HEX) + F(" (") + sensor_name + ')');

  setupWiFi();
  Serial.println(F("WiFi started."));
  setupArduinoOTA();
  Serial.println(F("ArduinoOTA started."));
  setupDHT();
  Serial.println(F("DHT started."));
}

void loop() {
  handleWiFi();
  handleArduinoOTA();
  handleHTTPServer();
  if (millis() >= next_post_timestamp) {
    next_post_timestamp = millis() + POST_INTERVAL;
    if (destination == String(PSTR("carbon"))) {
      handleCarbon();
    } else if (destination == String(PSTR("https"))) {
      handleHTTPSClient();
    }
  }
  yield();
}
