#include "SensorReading.h"
#include "SensorSampler.h"
#include "sdi12.h"
#include "SdCard.h"
#include "DataLogger.h"
#include "AvgDataLogger.h"
#include "Dashboard.h"

#define DIRO_PIN 8
#define MY_ADDRESS '0'

// Initialise sensors, SD card, loggers, dashboard and SDI-12 communication
void setup() {
  Serial.begin(9600);

  sensorsInit();
  sdCardInit();
  sensorSamplerInit();
  dataLoggerInit();
  avgDataLoggerInit();
  dashboardInit();
  sdi12Init(MY_ADDRESS, DIRO_PIN);

  readSensors();

  Serial.println(F("SDI-12 slave + ISR sampler + loggers + dashboard ready."));
}

// Continuously run sampler, SDI-12 handler, data loggers and dashboard
void loop() {
  sensorSamplerService();

  sdi12Handle();
  avgDataLoggerUpdate();
  dataLoggerUpdate();
  dashboardUpdate();
}