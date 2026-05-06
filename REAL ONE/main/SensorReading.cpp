#include "SensorReading.h"
#include <Wire.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

static SensorData sensorBuffer;

void sensorsInit() {
  Wire.begin();
  lightMeter.begin();
  bme.begin(0x76);
  sensorBuffer.ready = false;
}

void readSensors() {
  sensorBuffer.temperature = bme.readTemperature();
  sensorBuffer.humidity     = bme.readHumidity();
  sensorBuffer.pressure     = bme.readPressure() / 100.0;
  sensorBuffer.lux          = lightMeter.readLightLevel();
  sensorBuffer.ready        = true;

  Serial.println("[Sensors] Reading:");
  Serial.print("  Temp: ");     Serial.println(sensorBuffer.temperature);
  Serial.print("  Humidity: "); Serial.println(sensorBuffer.humidity);
  Serial.print("  Pressure: "); Serial.println(sensorBuffer.pressure);
  Serial.print("  Lux: ");      Serial.println(sensorBuffer.lux);
}

SensorData getSensorData() {
  return sensorBuffer;
}