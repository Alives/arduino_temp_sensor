#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1020.0)
#define BME_PIN 0x76

// Constants for Magnus formula
#define MAGNUS_A (17.27)  // constant (dimensionless)
#define MAGNUS_B (237.7)  // constant for temperature (°C)

Adafruit_BME280 bme;

float calculateDewPoint() {
  // Calculate dew point in Celsius using Magnus-Tetens approximation
  float alpha = (
    (MAGNUS_A * env.celcius) / (MAGNUS_B + env.celcius)
  ) + log(env.humidity / 100.0);
  float dewPointC = (MAGNUS_B * alpha) / (MAGNUS_A - alpha);

  return dewPointC;
}

float convertTempCtoF(float celcius) {
  return (celcius * 1.8F) + 32.0F;
}

void setupBME() {
  bme.begin(BME_PIN);
}

void updateEnvironment() {
  env.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  env.celcius = bme.readTemperature();
  env.fahrenheit = convertTempCtoF(env.celcius);
  env.humidity = bme.readHumidity();
  env.pressure = bme.readPressure() / 100.0F;
  env.dewpointC = calculateDewPoint();
  env.dewpointF = convertTempCtoF(env.dewpointC);
}
