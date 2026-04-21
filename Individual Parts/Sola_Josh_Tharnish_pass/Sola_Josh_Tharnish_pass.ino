#include <BH1750.h>
#include <Wire.h>

#include <Adafruit_BME280.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  lightMeter.begin();
  bme.begin(0x76);
}



void loop() {
  readSensors();
  delay(1000);
}


void readSensors() {
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0;
  float light = lightMeter.readLightLevel();

  Serial.print(temperature);
  Serial.println(" Celcius");
  Serial.print(humidity);
  Serial.println(" % humidity");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print(light);
  Serial.println(" Lux");
}