#include <ArduinoOTA.h>

void setupArduinoOTA() {
  ArduinoOTA.onStart([]() { Serial.println("ArduinoOTA Started"); });

  ArduinoOTA.onEnd([]() { Serial.println("ArduinoOTA Ended"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.print("ArduinoOTA Progress: ");
    Serial.print(String(progress / (total / 100.0F)));
    Serial.print("%\r");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print("ArduinoOTA Error[" + String(error) + "]: ");
    if (error == OTA_AUTH_ERROR)
      Serial.println("ArduinoOTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("ArduinoOTA Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("ArduinoOTA Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("ArduinoOTA Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("ArduinoOTA End Failed");
  });

  ArduinoOTA.setHostname(sensor_name.c_str());
  ArduinoOTA.setPassword(ota_password.c_str());
  ArduinoOTA.begin();
}

void handleArduinoOTA() {
  ArduinoOTA.handle();
}
