#define VERSION "6.0"
#define HOST "change-this-value!!" ERROR HERE TO FORCE USER TO UPDATE VALUE
#define HTTP_POST_INTERVAL 5000
#define HTTPS_PORT 443

/*  6.0:
 *   - data now in concise format instead of json for less bandwidth use.
 *   - No more syslog.
 *
 * 5.0:
 *   - No 404, all URIs result in metrics output.
 *   - struct for temps and errors.
 *   - split all functions into specific files.
 *   - Use F()/PSTR() macros everywhere.
 *   - No syslog for OTA since it only outputs to Serial.
 *
 *  4.0:
 *   - Fixed issue where syslog was sent twice before clearing.
 *   - Also push log via HTTPS.
 *   - Clean up includes.
 *   - POST with sections designated by <> tags and data in json format.
 *   - Add heap and RSSI metrics.
 *   - Convert strings to F() where possible.
 *
 *  3.0:
 *  - Push data via https.
 *  - Use millis() for intervals and handle rollovers.
 *  - Added /reset function (though it probably doesn't work well).
 *  - Add more metrics to json output.
 *  - Return non-string json values as their own types (float/int).
 *  - Changed post interval to >= 5 seconds.
 *
 * 2.0:
 *  - Added WifiManager
 */

struct env_t { float c, f, h; } env;
struct sensor_errors_t { uint8_t humidity, temperature; } errors;

uint32_t https_connect_attempts = 0L;
uint32_t next_post_timestamp = 0L;

String host, sensor_name, ssid;

void setSensorName () {
  struct sensor_s {
    const int sensor_id;
    const char * sensor_name;
  };
  sensor_s sensors[] = {
    {0xc6e1ff, PSTR("1")},
    {0x92ea79, PSTR("2")},
    {0xd65b01, PSTR("kitchen")},
    {0xd658f7, PSTR("travel")},
    {0xd65878, PSTR("outside")},
    {0x068231, PSTR("bedroom")},
  };
  int id = ESP.getChipId();

  sensor_name = PSTR("ERROR");
  for (unsigned int i = 0; i < sizeof(sensors); i++) {
    if (sensors[i].sensor_id == id) {
      sensor_name = sensors[i].sensor_name;
      break;
    }
  }
}

void setup () {
  Serial.begin(19200);
  Serial.println();
  delay(100);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);

  errors = {0,0};
  host = PSTR(HOST);

  Serial.print(F("Version "));
  Serial.println(F(VERSION));
  setSensorName();
  Serial.print(F("Chip ID: 0x"));
  Serial.println(String(ESP.getChipId(), HEX) + F(" (") + sensor_name + ')');

  setupArduinoOTA();
  Serial.println(F("ArduinoOTA started."));
  setupDHT();
  Serial.println(F("DHT started."));
  setupHTTPServer();
  Serial.println(F("HTTP server started."));
  setupWiFi();
  Serial.println(F("WiFi started."));
}

void loop () {
  handleWiFi();
  handleArduinoOTA();
  handleHTTPServer();
  handleHTTPSClient();
  yield();
}
