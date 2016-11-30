#include <Adafruit_Sensor.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <EspSaveCrash.h>
#include <FS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>


#define DHTPIN 5
#define DHTTYPE DHT22
#define HTTP_PORT 80
#define VERSION "2.0"


const char* ssid = "";  // Replace this.
const char* psk = "";   // Replace this.
const String sensor_array[] = {
  "c6e1ff", "92ea79", "d65b01", "d658f7", "d65878", "068231"};
bool setupComplete = false;
bool wifiConnected = false;
uint32_t delayMS;
uint32_t lastRead;
uint32_t humidityErrors = 0;
uint32_t temperatureErrors = 0;
String syslog = "";


DHT_Unified dht(DHTPIN, DHTTYPE);
ESP8266WebServer HTTPserver(HTTP_PORT);


void setupDHT(void) {
  sensor_t sensor;
  String name;
  String unit;

  dht.begin();
  writeSyslog("DHT" + String(DHTTYPE) + " Sensor ready.");
  writeSyslog("------------ DHT Sensor ------------");
  for(int i = 0; i < 2; i++) {
    if (i == 0) {
      dht.temperature().getSensor(&sensor);
      name = "Temperature";
      unit = "°C";
    } else {
      dht.humidity().getSensor(&sensor);
      name = "Humidity";
      unit = "%";
    }
    writeSyslog(name + " Sensor:     " + String(sensor.name));
    writeSyslog(name + " Driver Ver: " + String(sensor.version));
//  writeSyslog(name + " Unique ID:  " + String(sensor.sensor_id));
    writeSyslog(name + " Max Value:  " + String(sensor.max_value) + unit);
    writeSyslog(name + " Min Value:  " + String(sensor.min_value) + unit);
    writeSyslog(name + " Resolution: " + String(sensor.resolution) + unit);
  }
  writeSyslog("------------------------------------");

  delayMS = sensor.min_delay / 1000;
}


String getSensorData(void) {
  sensors_event_t event;
  uint32_t extraDelay;
  uint32_t timeNow = millis();
  String c;
  String f;
  String h;
  String response = "{";

  dht.temperature().getEvent(&event);
  while (isnan(event.temperature)) {
    temperatureErrors += 1;
    printDebug("Error reading temperature!");
    delay(250);
    dht.temperature().getEvent(&event);
  }

  c = String(event.temperature);
  f = String((event.temperature * 9.0 / 5.0) + 32);

  dht.humidity().getEvent(&event);
  while (isnan(event.relative_humidity)) {
    humidityErrors += 1;
    printDebug("Error reading humidity!");
    delay(250);
    dht.humidity().getEvent(&event);
  }
  lastRead = millis();
  h = String(event.relative_humidity);

  response += "\"tempF\": \"" + f + "\", " +
              "\"tempC\": \"" + c + "\", " +
              "\"humidity\": \"" + h + "\"}\n";

  printDebug(timestamp() + "Humidity: " + h + "%  " +
             "Temperature: " + c + "C " + f + "F");

  return response;
}


void getRoot(void) {
  HTTPserver.send(200, "text/plain", getSensorData());
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
  writeSyslog("HTTP/404 " + HTTPserver.uri());
}


void ledsOn(void) {
  digitalWrite(0, 0);
  digitalWrite(2, 0);
  HTTPserver.send(200, "text/plain", "LEDs are on.\n");
}


void ledsOff(void) {
  digitalWrite(0, 1);
  digitalWrite(2, 1);
  HTTPserver.send(200, "text/plain", "LEDs are off.\n");
}


void getLog(void) {
  String response = syslog;
  response += "\nUptime: " + String(millis() / 1000) + " seconds\n";
  response += "Humidity errors: " + String(humidityErrors) + "\n";
  response += "Temperature errors: " + String(temperatureErrors) + "\n";
  response += "\n" + getSensorData() + "\n";
  HTTPserver.send(200, "text/plain; charset=\"UTF-8\"", response);
}


void handleWiFi(void) {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  } else if (setupComplete) {
    writeSyslog("WiFi disconnected.  Reconnecting...");
  }
  int onoff = 0;

  digitalWrite(2, 0);
  writeSyslog("Starting WiFi");
  WiFi.begin(ssid, psk);

  writeSyslog("Connecting to SSID: " + WiFi.SSID());
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, onoff);
    onoff ^= 1;
    delay(25);
  }
  digitalWrite(2, 1);

  writeSyslog("Connected!");
  writeSyslog("RSSI: " + String(WiFi.RSSI()) + "dBm");
  writeSyslog("IP address: " + WiFi.localIP().toString());
  writeSyslog("Subnet mask: " + WiFi.subnetMask().toString());
  writeSyslog("Gateway: " + WiFi.gatewayIP().toString());
  wifiConnected = true;
}


String timestamp(void) {
  return "[" + String(millis() / 1000.0) + "] ";
}


void writeSyslog(const String line) {
  String entry = timestamp() + line;
  syslog += entry + "\n";
  printDebug(entry);
}


void printDebug(const String line) {
  Serial.println(line);
}


void setupArduinoOTA(String sensorName) {
  ArduinoOTA.onStart([]() {
    writeSyslog("ArduinoOTA Started");
  });
  ArduinoOTA.onEnd([]() {
    writeSyslog("ArduinoOTA Ended");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    writeSyslog("ArduinoOTA Progress: " +
        String(progress / (total / 100)) + "\r");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    String msg = "ArduinoOTA Error[" + String(error) + "]: ";
    if (error == OTA_AUTH_ERROR) msg += "ArduinoOTA Auth Failed";
    else if (error == OTA_BEGIN_ERROR) msg += "ArduinoOTA Begin Failed";
    else if (error == OTA_CONNECT_ERROR) msg += "ArduinoOTA Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) msg += "ArduinoOTA Receive Failed";
    else if (error == OTA_END_ERROR) msg += "ArduinoOTA End Failed";
    writeSyslog(msg);
  });

  ArduinoOTA.setHostname(sensorName.c_str());
  ArduinoOTA.setPassword(psk);
  ArduinoOTA.begin();
  writeSyslog("ArduinoOTA started");
}


String sensorNumber(void) {
  String id = String(ESP.getChipId(), HEX);
  for (int i = 0; i < sizeof(sensor_array); i++) {
    if (sensor_array[i] == id) return String(i + 1);
  }
  return "detection error!";
}


void setup(void) {
  String sensorName = "Sensor" + sensorNumber();
  Serial.begin(115200);
  delay(100);

  writeSyslog("Sensor " + sensorNumber());
  writeSyslog("Code version " + String(VERSION));
  writeSyslog("Chip ID: 0x" + String(ESP.getChipId(), HEX));

  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);

  writeSyslog("MAC address: " + WiFi.macAddress());

  setupDHT();

  WiFi.hostname(sensorName.c_str());

  if (SPIFFS.begin()) {
    writeSyslog("Mounted file system");
  } else {
    writeSyslog("Error mounting file system");
    return;
  }

  handleWiFi();

  if (!MDNS.begin(sensorName.c_str())) {
    writeSyslog("Error setting up MDNS responder!");
  }
  writeSyslog("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  setupArduinoOTA(sensorName);

  HTTPserver.on("/", getRoot);
  HTTPserver.on("/log", getLog);
  HTTPserver.on("/leds_on", ledsOn);
  HTTPserver.on("/leds_off", ledsOff);
  HTTPserver.onNotFound(notFound);
  HTTPserver.begin();
  writeSyslog("HTTP server started on port " + String(HTTP_PORT));
  setupComplete = true;
}


void loop(void) {
  ArduinoOTA.handle();
  HTTPserver.handleClient();
  handleWiFi();
  yield();
}
