#include "SensorReading.h"
#include "SensorSampler.h"
#include "sdi12.h"
#include "SdCard.h"
#include "DataLogger.h"
#include "AvgDataLogger.h"
#include "Dashboard.h"
#include "Watchdog.h"

#define DIRO_PIN 8
#define MY_ADDRESS '0'

// Prevent Arduino core from disabling watchdog
void watchdogSetup(void)
{
}

void setup()
{
  Serial.begin(9600);

  sensorsInit();
  sdCardInit();
  sensorSamplerInit();
  dataLoggerInit();
  avgDataLoggerInit();
  dashboardInit();
  sdi12Init(MY_ADDRESS, DIRO_PIN);

  readSensors();

  watchdogInit();

  Serial.println(F("SDI-12 Communication + ISR sampler + loggers + dashboard ready."));
}

void loop()
{
  watchdogKick();

  sensorSamplerService();
  watchdogKick();

  watchdogCheckSensors();
  watchdogKick();

  sdi12Handle();
  watchdogKick();

  avgDataLoggerUpdate();
  watchdogKick();

  dataLoggerUpdate();
  watchdogKick();

  dashboardUpdate();
  watchdogKick();
}