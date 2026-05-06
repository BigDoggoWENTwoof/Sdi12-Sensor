#ifndef SENSORREADING_H
#define SENSORREADING_H

#include <BH1750.h>
#include <Adafruit_BME280.h>

struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  float lux;
  bool  ready;
};

void sensorsInit();
void readSensors();
SensorData getSensorData();

#endif