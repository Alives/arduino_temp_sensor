#include <ESP8266WebServer.h>

ESP8266WebServer http_server(80);

void httpRoot () {
  String msg = PSTR("HTTP GET ");
  msg += http_server.uri();
  syslog(msg);
  http_server.send(200, F("text/plain"), getSensorData());
}

void httpNoOp() { return; }  // Don't do anything.

void setupHTTPServer () {
  http_server.on(F("/favicon.ico"), httpNoOp);
  http_server.onNotFound(httpRoot);
  http_server.begin();
}

void handleHTTPServer () {
  http_server.handleClient();
}
