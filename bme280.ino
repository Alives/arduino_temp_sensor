#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1020.0)
#define BME_PIN 0x76

Adafruit_BME280 bme;

void setupBME() {
  bme.begin(BME_PIN);
}

void updateEnvironment() {
  env.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  env.celcius = bme.readTemperature();
  env.fahrenheit = (env.celcius * 1.8F) + 32.0F;
  env.humidity = bme.readHumidity();
  env.pressure = bme.readPressure() / 100.0F;
}
