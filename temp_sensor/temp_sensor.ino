#define VERSION "5.0"
#define HOST "CHANGE ME CHANGE ME CHANGE ME CHANGE ME"
#define HTTP_POST_INTERVAL 5000

/*  5.0:
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

int sensor_id = -1;

String getSensorName () {
  struct sensor_s {
    const int sensor_id;
    const __FlashStringHelper* sensor_name;
  };
  sensor_s sensors[] = {
    {0xc6e1ff, F("1")},
    {0x92ea79, F("2")},
    {0xd65b01, F("kitchen")},
    {0xd658f7, F("travel")},
    {0xd65878, F("outside")},
    {0x068231, F("bedroom")},
  };
  if (sensor_id >= 0) {
    return PSTR("sensor_") + String(sensors[sensor_id].sensor_name);
   }

  int id = ESP.getChipId();
  for (unsigned int i = 0; i < sizeof(sensors); i++) {
    if (sensors[i].sensor_id == id) {
      sensor_id = i;
      return getSensorName();
    }
  }
  syslog(F("Sensor ID detection error!"));
  return PSTR("ERROR_sensor_name");
}

void setup () {
  Serial.begin(19200);
  Serial.println();
  delay(100);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);

  syslog(String(PSTR("temp_sensor version ")) + PSTR(VERSION));
  syslog(PSTR("Chip ID: 0x") + String(ESP.getChipId(), HEX));
  syslog(getSensorName());

  setupArduinoOTA();
  syslog(F("ArduinoOTA started."));
  setupDHT();
  syslog(F("DHT Sensor started."));
  setupHTTPSClient();
  syslog(F("HTTPS client started."));
  setupHTTPServer();
  syslog(F("HTTP server started."));
  setupWiFi();
  syslog(F("WiFi started."));
}

void loop () {
  handleWiFi();
  handleArduinoOTA();
  handleHTTPServer();
  handleHTTPSClient();
  yield();
}
