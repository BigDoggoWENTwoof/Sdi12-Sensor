#include "SensorReading.h"
#include "SensorSampler.h"
#include "sdi12.h"
#include "SdCard.h"
#include "DataLogger.h"
#include "AvgDataLogger.h"
#include "Dashboard.h"
#include "Watchdog.h"

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
  watchdogInit();  // Enable auto-recovery if firmware hangs.

  Serial.println(F("SDI-12 slave + ISR sampler + loggers + dashboard ready."));
}

void loop() {
  watchdogKick();  // Start-of-loop heartbeat.

  // Drain hardware timer flags and run I2C reads (timing set by TC4, not loop speed).
  sensorSamplerService();
  watchdogKick();  // Keep watchdog fed between major tasks.

  sdi12Handle();
  watchdogKick();  // Keep watchdog fed between major tasks.
  avgDataLoggerUpdate();
  watchdogKick();  // Keep watchdog fed between major tasks.
  dataLoggerUpdate();
  watchdogKick();  // Keep watchdog fed between major tasks.
  dashboardUpdate();
  watchdogKick();  // End-of-loop heartbeat.
}
