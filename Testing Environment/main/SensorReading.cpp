#include "SensorReading.h"
#include <Wire.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

bool bmeOK = false;
bool lightOK = false;

static SensorData sensorBuffer;


// Sets up the I2C bus and checks if the BME280 and BH1750 sensors are connected.
void sensorsInit() {
  Wire.begin();

  
  lightOK = lightMeter.begin();
  bmeOK   = bme.begin(0x76);

  Serial.print("[Init] BME280: ");
  Serial.println(bmeOK ? "OK" : "FAIL");

  Serial.print("[Init] BH1750: ");
  Serial.println(lightOK ? "OK" : "FAIL");

  sensorBuffer.ready = false;
}


// Reads data from the BME280 and BH1750 sensors and stores the latest values in sensorBuffer.
void readSensors() {

  float pres = bme.readPressure();

  if (isnan(pres) || pres <= 0) {
    bmeOK = false;
  }
  else {
    bmeOK = true;

    sensorBuffer.temperature = bme.readTemperature();
    sensorBuffer.humidity    = bme.readHumidity();
    sensorBuffer.pressure    = pres / 100.0;
  }

  
  float lux = lightMeter.readLightLevel();

  if (isnan(lux) || lux < 0) {
    lightOK = false;
  }
  else {
    lightOK = true;
    sensorBuffer.lux = lux;
  }

  
  sensorBuffer.ready = (bmeOK || lightOK);



}

// Returns the latest stored sensor readings from sensorBuffer.
SensorData getSensorData() {
  return sensorBuffer;
}

// Counts how many sensor values are currently available from the working sensors.

int getParameterCount() {
  int count = 0;

  if (bmeOK) {
    count += 3; 
  }

  if (lightOK) {
    count += 1; 
  }

  return count;
}

// Returns true if the BME280 temperature, humidity, and pressure sensor is working.

bool isBme280Ok() {
  return bmeOK;
}


// Returns true if the BH1750 light sensor is working.
bool isBh1750Ok() {
  return lightOK;
}