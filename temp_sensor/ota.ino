#include <ArduinoOTA.h>

void setupArduinoOTA () {
  ArduinoOTA.onStart ([]() { syslog(F("ArduinoOTA Started")); });

  ArduinoOTA.onEnd ([]() { syslog(F("ArduinoOTA Ended")); });

  ArduinoOTA.onProgress ([](unsigned int progress, unsigned int total) {
    // OTA pauses execution, syslog won't be sent.
    Serial.print(F("ArduinoOTA Progress: "));
    Serial.print(String(progress / (total / 100.0)));
    Serial.print(F("%\r"));
  });

  ArduinoOTA.onError ([](ota_error_t error) {
    // OTA pauses execution, syslog won't be sent.
    Serial.print(F("ArduinoOTA Error["));
    Serial.print(String(error));
    Serial.print(F("]: "));
    if (error == OTA_AUTH_ERROR) Serial.println(F("ArduinoOTA Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("ArduinoOTA Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("ArduinoOTA Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("ArduinoOTA Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("ArduinoOTA End Failed"));
  });

  ArduinoOTA.setHostname(getSensorName().c_str());
  ArduinoOTA.setPassword(PSTR("password"));
  ArduinoOTA.begin();
}

void handleArduinoOTA () {
  ArduinoOTA.handle();
}
