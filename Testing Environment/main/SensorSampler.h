#ifndef SENSORSAMPLER_H  // Include guard start
#define SENSORSAMPLER_H  // Define guard macro

#include "SensorReading.h"  // SensorData struct and readSensors()

// HW timer ISR requests a sample every SensorSamplerInMs (2 s); call sensorSamplerService() from loop().
static void setActivityLed(bool on);
static void onSampleTimerISR();
static void serviceActivityLed();
static void accumulateOneSample();
static void finalizeAverageWindow();
static void pulseActivityLed();
static void printAverageReady();

void sensorSamplerInit();  // Start Arduino Due DueTimer (10 ms tick -> 2 s sample)
void sensorSamplerService();  // Process pending ISR ticks: read sensors and build averages

SensorData getAveragedSensorData();  // Last completed 2 s average (for logging only)
bool isAveragedSensorDataReady();  // True when a new average is ready to log
void sensorSamplerAcknowledgeAverage();  // Clear ready flag after AvgDataLogger writes CSV row

#endif  // Include guard end
