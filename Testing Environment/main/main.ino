#include "SensorReading.h"
#include "SensorSampler.h"
#include "sdi12.h"
#include "SdCard.h"
#include "DataLogger.h"
#include "AvgDataLogger.h"
#include "Dashboard.h"

#define DIRO_PIN 8   // pin 7 is TFT_DC
#define MY_ADDRESS '0'

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

void loop() {
  // Drain hardware timer flags and run I2C reads (timing set by TC4, not loop speed).
  sensorSamplerService();

  sdi12Handle();
  avgDataLoggerUpdate();
  dataLoggerUpdate();
  dashboardUpdate();
}
