#include <DHT.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

int http_port = 80;
String VERSION = "1.1";
#define DHTPIN 13
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
const char* ssid = "--------";
const char* password = "--------";
MDNSResponder mdns;
ESP8266WebServer server(http_port);

void logConsole(float h, float c, float f) {
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %       ");
  Serial.print("Temperature: ");
  Serial.print(c);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.println(" *F");
}

void root() {
  String response = "";
  char buf1[80];
  char buf2[16];
  float h = dht.readHumidity();
  float c = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(c)) {
    Serial.println("Error reading temperature!");
  } else if (isnan(h)) {
    Serial.println("Error reading humidity!");
  }

  dtostrf(f, 5, 2, buf2);
  sprintf(buf1, "{\"tempF\": \"%s\", ", buf2);
  response += buf1;
  dtostrf(c, 5, 2, buf2);
  sprintf(buf1, "\"tempC\": \"%s\", ", buf2);
  response += buf1;
  dtostrf(h, 5, 2, buf2);
  sprintf(buf1, "\"humidity\": \"%s\", ", buf2);
  response += buf1;
  response += "\"version\": \"" + VERSION + "\"}\n";

  logConsole(h, c, f);
  server.send(200, "text/plain", response);
}

void notFound(void) {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void ledsOn(void) {
  digitalWrite(0, 0);
  digitalWrite(2, 0);
  server.send(200, "text/plain", "LEDs are on.");
}

void ledsOff(void) {
  digitalWrite(0, 1);
  digitalWrite(2, 1);
  server.send(200, "text/plain", "LEDs are off.");
}

void setup(void) {
  int onoff = 0;
  Serial.begin(115200);
  dht.begin();
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, 1);
  digitalWrite(2, 0);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, onoff);
    delay(500);
    Serial.print(".");
    onoff ^= 1;
  }
  digitalWrite(2, 1);

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.on("/", root);
  server.on("/leds_on", ledsOn);
  server.on("/leds_off", ledsOff);
  server.onNotFound(notFound);
  server.begin();
  Serial.print("HTTP server started on port ");
  Serial.println(http_port);
}

void loop(void) {
  server.handleClient();
}
