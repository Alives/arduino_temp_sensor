#include <DHT_U.h>

#define DHTPIN 5
#define DHTTYPE DHT22

struct env_t { float c, f, h; };
struct sensor_errors_t { uint32_t humidity, temperature; };

DHT_Unified dht(DHTPIN, DHTTYPE);
struct sensor_errors_t errors = { 0L , 0L };

struct env_t getEnvironment() {
  env_t env;
  env.c = getTemperature();
  env.f = (float) (env.c * 1.8) + 32;
  env.h = getHumidity();
  return env;
}

float getTemperature() {
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  while (isnan(event.temperature)) {
    errors.temperature += 1;
    syslog(F("Error reading temperature!"));
    delay(250);
    dht.temperature().getEvent(&event);
  }
  return event.temperature;
}

float getHumidity() {
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  while (isnan(event.relative_humidity)) {
    errors.humidity += 1;
    syslog(F("Error reading humidity!"));
    delay(250);
    dht.humidity().getEvent(&event);
  }
  return event.relative_humidity;
}

void setupDHT () {
  dht.begin();
}
