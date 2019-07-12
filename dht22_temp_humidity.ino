/*  5.0:
 *   - AutoConnect instead of WifiManager.
 *   - No 404, all URIs result in metrics output.
 *   - struct for temps.
 *
 *  4.0:
 *   - Fixed issue where syslog was sent twice before clearing.
 *   - Also push log via HTTPS.
 *   - Clean up includes.
 *   - POST with sections designated by <> tags and data in json format.
 *   - Add heap and RSSI metrics
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

#include <ArduinoOTA.h>
#include <AutoConnect.h>
#include <DHT_U.h>
#include <WiFiClientSecure.h>

#define DHTPIN 5
#define DHTTYPE DHT22
#define POST_INTERVAL 5000
#define VERSION "3.0"


const PROGMEM char* host = "listener.domain.com" + ERROR -> UPDATE THIS!
const PROGMEM int https_port = 443;
const PROGMEM String sensor_array[] = {
  "c6e1ff", "92ea79", "d65b01", "d658f7", "d65878", "068231"};
const PROGMEM String sensor_names[] = {
  "1", "2", "kitchen", "travel", "outside", "bedroom"};
String sensor_name = "";
String log_str = "";

uint32_t https_connect_attempts = 0L;
uint32_t humidity_errors = 0L;
uint32_t next_post_timestamp = 0L;
uint32_t temperature_errors = 0L;

DHT_Unified dht(DHTPIN, DHTTYPE);
ESP8266WebServer http_server(80);
WiFiClientSecure https_client;
AutoConnect portal(http_server);

typedef struct {
  float c;
  float f;
} temperatures;

void syslog (const String line) {
  String entry = '[' + String(millis() / 1000.0) + ']' + ' ' + line;
  Serial.println(entry);
  log_str += entry + '\n';
}

temperatures getTemperature () {
  float c;
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  while (isnan(event.temperature)) {
    temperature_errors += 1;
    syslog(F("Error reading temperature!"));
    delay(250);
    dht.temperature().getEvent(&event);
  }
  c = event.temperature;
  return { c , (float) (c * 1.8) + 32 };
}

float getHumidity () {
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  while (isnan(event.relative_humidity)) {
    humidity_errors += 1;
    syslog(F("Error reading humidity!"));
    delay(250);
    dht.humidity().getEvent(&event);
  }
  return event.relative_humidity;
}

String getSensorData () {
  const String comma = F(", ");
  temperatures temps = getTemperature();
  String data = F("{");
  data += F("\"free_heap\": ");
  data += String(ESP.getFreeHeap());
  data += comma;
  data += F("\"heap_fragmentation_percent\": ");
  data += String(ESP.getHeapFragmentation());
  data += comma;
  data += F("\"https_connect_attempts\": ");
  data += String(https_connect_attempts);
  data += comma;
  data += F("\"humidity_errors\": ");
  data += String(humidity_errors);
  data += comma;
  data += F("\"humidity\": ");
  data += String(getHumidity());
  data += comma;
  data += F("\"RSSI\": ");
  data += String(WiFi.RSSI());
  data += comma;
  data += F("\"temperature_errors\": ");
  data += String(temperature_errors);
  data += comma;
  data += F("\"tempC\": ");
  data += String(temps.c);
  data += comma;
  data += F("\"tempF\": ");
  data += String(temps.f);
  data += comma;
  data += F("\"uptimeMS\": ");
  data += String(millis());
  data += F("}");
  return data;
}

void httpRoot () {
  const String msg = F("HTTP GET ");
  syslog(msg + http_server.uri());
  http_server.send(200, F("text/plain"), getSensorData());
}

void httpLedsOn () {
  const String msg = F("LEDs are on.");
  digitalWrite(0, 0);
  digitalWrite(2, 0);
  syslog(msg);
  http_server.send(200, F("text/plain"), msg);
}

void httpLedsOff () {
  const String msg = F("LEDs are off.");
  digitalWrite(0, 1);
  digitalWrite(2, 1);
  syslog(msg);
  http_server.send(200, F("text/plain"), msg);
}

void httpNoOp() {}  // Don't do anything.

void httpReset () {
  const String msg = F("Resetting system...");
  syslog(msg);
  http_server.send(200, F("text/plain"), msg);
  systemReset();
}

void systemReset () {
  ESP.restart();
  delay(5000);
}

void httpPost () {
  postData();
  httpRoot();
}

void writeGraphite () {
  if (millis() >= next_post_timestamp) {
    next_post_timestamp = millis() + POST_INTERVAL;
    postData();
  }
}

void connectHTTPSClient() {
  const String conn = F("Connecting to https://");
  for (;;) {
    syslog(conn + String(host));
    https_client.setInsecure();
    if (https_client.connect(host, https_port)) {
      syslog(F("HTTPS client connected!"));
      https_connect_attempts += 1;
      https_client.keepAlive();
      return;
    } else {
      syslog(conn + String(host) + F(" failed!"));
      delay(5000);
    }
  }
}

void postData () {
  if (WiFi.status() != WL_CONNECTED) { return; }
  const String rn = F("\r\n");
  String data;
  String headers;
  data = F("<prefix>temp_sensor.");
  data += sensor_name;
  data += F("</prefix><data>");
  data += getSensorData();
  data += F("</data><log>");
  data += log_str;
  data += F("</log>");

  if (!https_client.connected()) {
    connectHTTPSClient();
  } else {
    headers = F("POST / HTTP/1.1\r\nHost: ");
    headers += String(host);
    headers += rn;
    headers += F("User-Agent: ");
    headers += sensor_name;
    headers += rn;
    headers += F("Content-Length: ");
    headers += String(data.length());
    headers += rn;
    headers += F("Connection: keep-alive\r\n\r\n");
    https_client.print(headers + data);
    log_str = F("");  // Clear the syslog.
  }
  if (https_client.available()) {
    String status = https_client.readStringUntil('\r');
    https_client.readString();  // flush the buffer.
    if (status != F("HTTP/1.1 200 OK")) {
      syslog(status);
    }
  }
}

void setupArduinoOTA (String sensor_name) {
  ArduinoOTA.onStart([]() {
    syslog(F("ArduinoOTA Started"));
  });
  ArduinoOTA.onEnd([]() {
    syslog(F("ArduinoOTA Ended"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    String msg = F("ArduinoOTA Progress: ");
    msg += String(progress / (total / 100.0));
    msg += '\r';
    syslog(msg);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    String msg = F("ArduinoOTA Error[");
    msg += String(error);
    msg += F("]: ");
    if (error == OTA_AUTH_ERROR) msg += F("ArduinoOTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR) msg += F("ArduinoOTA Begin Failed");
    else if (error == OTA_CONNECT_ERROR) msg += F("ArduinoOTA Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) msg += F("ArduinoOTA Receive Failed");
    else if (error == OTA_END_ERROR) msg += F("ArduinoOTA End Failed");
    syslog(msg);
  });

  ArduinoOTA.setHostname(sensor_name.c_str());
  ArduinoOTA.setPassword("password");
  ArduinoOTA.begin();
  syslog(F("ArduinoOTA started"));
}

void setSensorName () {
  String id = String(ESP.getChipId(), HEX);
  for (unsigned int i = 0; i < sizeof(sensor_array); i++) {
    if (sensor_array[i] == id) {
      sensor_name = F("sensor_");
      sensor_name += sensor_names[i];
      return;
    }
  }
  syslog(F("Sensor name detection error!"));
}

bool startCaptivePortal(__attribute__((unused)) IPAddress ip) {
  int onoff = 0;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, onoff);
    onoff ^= 1;
    delay(25);
  }
  return true;
}

void setup () {
  String msg;
  String ssid;
  setSensorName();
  Serial.begin(115200);
  delay(100);

  syslog(sensor_name);
  msg = F("Code version ");
  syslog(msg + String(VERSION));
  msg = F("Chip ID: 0x");
  syslog(msg + String(ESP.getChipId(), HEX));

  http_server.on(F("/favicon.ico"), httpNoOp);
  http_server.on(F("/leds_on"), httpLedsOn);
  http_server.on(F("/leds_off"), httpLedsOff);
  http_server.on(F("/post"), httpPost);
  http_server.on(F("/reset"), httpReset);
  http_server.on(F("/reboot"), httpReset);
  portal.onNotFound(httpRoot);  // Just show default page.

  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);
  digitalWrite(2, 0);

  msg = F("MAC address: ");
  syslog(msg + WiFi.macAddress());
  WiFi.hostname(sensor_name);

  AutoConnectConfig config;
  ssid = sensor_name + F(" ") + WiFi.macAddress();
  config.apid = ssid.c_str();
  config.psk = F("");
  config.autoReconnect = true;
  config.portalTimeout = 60000;  // Timeout in 60s.
  portal.config(config);

  while (true) {
    msg = F("Attempting to connect to SSID: ");
    syslog(msg + WiFi.SSID());
    portal.onDetect(startCaptivePortal);
    if (portal.begin()) { break; }
    delay(250);
  }

  digitalWrite(2, 1);
  syslog(F("Connected!"));
  msg = F("RSSI: ");
  syslog(msg + String(WiFi.RSSI()) + F("dBm"));
  msg = F("IP address: ");
  syslog(msg + WiFi.localIP().toString());
  msg = F("Subnet mask: ");
  syslog(msg + WiFi.subnetMask().toString());
  msg = F("Gateway: ");
  syslog(msg + WiFi.gatewayIP().toString());

  setupArduinoOTA(sensor_name);
  dht.begin();
  syslog("DHT" + String(DHTTYPE) + " Sensor ready.");
}

void loop () {
  ArduinoOTA.handle();
  portal.handleClient();
  writeGraphite();
  yield();
}
