/*  3.0:
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

#include <Adafruit_Sensor.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <DHT_U.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <EspSaveCrash.h>
#include <FS.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

#define DHTPIN 5
#define DHTTYPE DHT22
#define HTTP_PORT 80
#define POST_INTERVAL 5000
#define VERSION "3.0"


const char* host = "<<<<<<<< CHANGE ME --  https-listener.domain.com >>>>>>>>>";
const int https_port = 443;
const String sensor_array[] = {
  "c6e1ff", "92ea79", "d65b01", "d658f7", "d65878", "068231"};
const String sensor_names[] = {
  "1", "2", "kitchen", "travel", "outside", "bedroom"};
bool setup_complete = false;
bool wifi_connected = false;
bool write_graphite_flag = false;

String sensor_name = "";
String log_str = "";

uint32_t https_connect_attempts = 0L;
uint32_t humidity_errors = 0L;
uint32_t next_post_timestamp = 0L;
uint32_t temperature_errors = 0L;

DHT_Unified dht(DHTPIN, DHTTYPE);
ESP8266WebServer HTTPserver(HTTP_PORT);
WiFiClientSecure https_client;


String timestamp(void) {
  return "[" + String(millis() / 1000.0) + "] ";
}

void syslog(const String line) {
  String entry = timestamp() + line;
  Serial.println(entry);
  log_str += entry + "\n";
}

void setupDHT(void) {
  sensor_t sensor;
  String name;
  String unit;

  dht.begin();
  syslog("DHT" + String(DHTTYPE) + " Sensor ready.");
  syslog("------------ DHT Sensor ------------");
  for (int i = 0; i < 2; i++) {
    if (i == 0) {
      dht.temperature().getSensor(&sensor);
      name = "Temperature";
      unit = "°C";
    } else {
      dht.humidity().getSensor(&sensor);
      name = "Humidity";
      unit = "%";
    }
    syslog(name + " Sensor:     " + String(sensor.name));
    syslog(name + " Driver Ver: " + String(sensor.version));
    syslog(name + " Max Value:  " + String(sensor.max_value) + unit);
    syslog(name + " Min Value:  " + String(sensor.min_value) + unit);
    syslog(name + " Resolution: " + String(sensor.resolution) + unit);
  }
  syslog("------------------------------------");
}

String getSensorData(void) {
  sensors_event_t event;
  String c;
  String f;
  String h;
  String response = "{";

  dht.temperature().getEvent(&event);
  while (isnan(event.temperature)) {
    temperature_errors += 1;
    syslog("Error reading temperature!");
    delay(250);
    dht.temperature().getEvent(&event);
  }

  c = String(event.temperature);
  f = String((event.temperature * 9.0 / 5.0) + 32);

  dht.humidity().getEvent(&event);
  while (isnan(event.relative_humidity)) {
    humidity_errors += 1;
    syslog("Error reading humidity!");
    delay(250);
    dht.humidity().getEvent(&event);
  }
  h = String(event.relative_humidity);

  response +=
    "\"https_connect_attempts\": " + String(https_connect_attempts) + ", " +
    "\"humidity_errors\": " + String(humidity_errors) + ", " +
    "\"humidity\": " + h + ", " +
    "\"temperature_errors\": " + String(temperature_errors) + ", " +
    "\"tempC\": " + c + ", " +
    "\"tempF\": " + f + ", " +
    "\"uptimeMS\": " + String(millis()) + "}";

  return response;
}

void httpRoot(void) {
  String data = getSensorData();
  syslog(timestamp() + "HTTP GET " + HTTPserver.uri() + " : " + data);
  HTTPserver.send(200, "text/plain", data + "\n");
}

void notFound(void) {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += HTTPserver.uri();
  message += "\nMethod: ";
  message += (HTTPserver.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += HTTPserver.args();
  message += "\n";
  for (uint8_t i = 0; i < HTTPserver.args(); i++) {
    message += " " + HTTPserver.argName(i) + ": " + HTTPserver.arg(i) + "\n";
  }
  HTTPserver.send(404, "text/plain", message);
  if (HTTPserver.uri() != "/favicon.ico") {
    syslog("HTTP/404 " + HTTPserver.uri());
  }
}

void httpLedsOn(void) {
  digitalWrite(0, 0);
  digitalWrite(2, 0);
  syslog("LEDs are on.");
  httpLog();
}

void httpLedsOff(void) {
  digitalWrite(0, 1);
  digitalWrite(2, 1);
  syslog("LEDs are off.");
  httpLog();
}

void httpLog(void) {
  String response = "\n" + timestamp() + "Current data: " + getSensorData();
  HTTPserver.send(200, "text/plain; charset=\"UTF-8\"", log_str + response);
}

void httpPost(void) {
  postData();
  httpLog();
}

void writeGraphite() {
  if (millis() >= next_post_timestamp) {
    next_post_timestamp = millis() + POST_INTERVAL;
    postData();
  }
}

void connectHTTPSClient(void) {
  for (;;) {
    syslog("Connecting to https://" + String(host));
    https_client.setInsecure();
    if (https_client.connect(host, https_port)) {
      syslog("HTTPS client connected!");
      https_connect_attempts += 1;
      https_client.keepAlive();
      return;
    } else {
      syslog("Connection to https://" + String(host) + " failed!");
      delay(5000);
    }
  }
}

void postData(void) {
  String data = "prefix=temp_sensor." + sensor_name +
                "&data=" + getSensorData();
  String data_length = String(data.length());

  if (!https_client.connected()) {
    connectHTTPSClient();
  } else {
    https_client.print(String(
      "POST / HTTP/1.1\r\n") +
      "Host: " + String(host) + "\r\n" +
      "User-Agent: " + sensor_name + "\r\n" +
      "Content-Length: " + data_length + "\r\n" +
      "Connection: keep-alive\r\n\r\n" + data);
  }
  if (https_client.available()) {
    String status = https_client.readStringUntil('\r');
    https_client.readString();  // flush the buffer.
    if (status != "HTTP/1.1 200 OK") {
      syslog(status);
    }
  }
}

void setupArduinoOTA(String sensor_name) {
  ArduinoOTA.onStart([]() {
    syslog("ArduinoOTA Started");
  });
  ArduinoOTA.onEnd([]() {
    syslog("ArduinoOTA Ended");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    syslog("ArduinoOTA Progress: " +
        String(progress / (total / 100.0)) + "\r");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    String msg = "ArduinoOTA Error[" + String(error) + "]: ";
    if (error == OTA_AUTH_ERROR) msg += "ArduinoOTA Auth Failed";
    else if (error == OTA_BEGIN_ERROR) msg += "ArduinoOTA Begin Failed";
    else if (error == OTA_CONNECT_ERROR) msg += "ArduinoOTA Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) msg += "ArduinoOTA Receive Failed";
    else if (error == OTA_END_ERROR) msg += "ArduinoOTA End Failed";
    syslog(msg);
  });

  ArduinoOTA.setHostname(sensor_name.c_str());
  ArduinoOTA.setPassword("password");
  ArduinoOTA.begin();
  syslog("ArduinoOTA started");
}

void setSensorName(void) {
  String id = String(ESP.getChipId(), HEX);
  for (int i = 0; i < sizeof(sensor_array); i++) {
    if (sensor_array[i] == id) {
      sensor_name = String("sensor_" + sensor_names[i]);
      return;
    }
  }
  syslog("Sensor name detection error!");
}

void httpReset(void) {
  syslog("Resetting system...");
  httpLog();
  systemReset();
}

void systemReset(void) {
  ESP.reset();
  delay(5000);
}

void handleWiFi(void) {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (setup_complete) {
    syslog("WiFi disconnected.  Reconnecting...");
  }
  int onoff = 0;

  digitalWrite(2, 0);
  syslog("Starting WiFi");

  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(30);
  String ssid = sensor_name + " " + WiFi.macAddress();
  if (!wifiManager.autoConnect(ssid.c_str())) {
    syslog("failed to connect and hit timeout");
    delay(3000);
    systemReset();
    delay(5000);
  }

  syslog("Connecting to SSID: " + WiFi.SSID());
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, onoff);
    onoff ^= 1;
    delay(25);
  }
  digitalWrite(2, 1);

  syslog("Connected!");
  syslog("RSSI: " + String(WiFi.RSSI()) + "dBm");
  syslog("IP address: " + WiFi.localIP().toString());
  syslog("Subnet mask: " + WiFi.subnetMask().toString());
  syslog("Gateway: " + WiFi.gatewayIP().toString());
  wifi_connected = true;
}

void setup(void) {
  setSensorName();
  Serial.begin(115200);
  delay(100);

  syslog(sensor_name);
  syslog("Code version " + String(VERSION));
  syslog("Chip ID: 0x" + String(ESP.getChipId(), HEX));

  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);

  syslog("MAC address: " + WiFi.macAddress());
  WiFi.hostname(sensor_name);

  handleWiFi();

  setupDHT();

  if (!MDNS.begin(sensor_name)) {
    syslog("Error setting up MDNS responder!");
  }
  syslog("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  setupArduinoOTA(sensor_name);

  HTTPserver.on("/", httpRoot);
  HTTPserver.on("/log", httpLog);
  HTTPserver.on("/leds_on", httpLedsOn);
  HTTPserver.on("/leds_off", httpLedsOff);
  HTTPserver.on("/post", httpPost);
  HTTPserver.on("/reset", httpReset);
  HTTPserver.on("/reboot", httpReset);
  HTTPserver.onNotFound(notFound);
  HTTPserver.begin();
  syslog("HTTP server started on port " + String(HTTP_PORT));

  setup_complete = true;
}

void loop(void) {
  ArduinoOTA.handle();
  HTTPserver.handleClient();
  handleWiFi();
  writeGraphite();
  yield();
}
