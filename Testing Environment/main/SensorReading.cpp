#include "SensorReading.h"
#include <Wire.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

bool bmeOK = false;
bool lightOK = false;

static SensorData sensorBuffer;

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

void readSensors() {
  
  float pres = bme.readPressure();

  if (pres < 0) {
    bmeOK = false;
  } else {
    bmeOK = true;
    sensorBuffer.temperature = bme.readTemperature();
    sensorBuffer.humidity    = bme.readHumidity();
    sensorBuffer.pressure    = pres / 100.0;
  }

  
  float lux = lightMeter.readLightLevel();

  if (lux < 0) {  
    lightOK = false;
  } else {
    lightOK = true;
    sensorBuffer.lux = lux;
  }

  
  sensorBuffer.ready = (bmeOK || lightOK);


}


SensorData getSensorData() {
  return sensorBuffer;
}

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

bool isBme280Ok() {
  return bmeOK;
}

bool isBh1750Ok() {
  return lightOK;
}