#include <DHT_U.h>

#define DHTPIN 5
#define DHTTYPE DHT22

DHT_Unified dht(DHTPIN, DHTTYPE);

void updateEnvironment() {
  updateTemperature();
  env.f = (float) (env.c * 1.8) + 32;
  updateHumidity();
}

void updateTemperature() {
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  while (isnan(event.temperature)) {
    errors.temperature += 1;
    Serial.println(F("Error reading temperature!"));
    delay(250);
    dht.temperature().getEvent(&event);
  }
  env.c = event.temperature;
}

void updateHumidity() {
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  while (isnan(event.relative_humidity)) {
    errors.humidity += 1;
    Serial.println(F("Error reading humidity!"));
    delay(250);
    dht.humidity().getEvent(&event);
  }
  env.h = event.relative_humidity;
}

void setupDHT() {
  dht.begin();
}
