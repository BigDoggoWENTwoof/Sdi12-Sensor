#ifndef SENSORSAMPLER_H  
#define SENSORSAMPLER_H  

#include "SensorReading.h"  

static void setActivityLed(bool state);
static void onSampleTimerISR();
static void serviceActivityLed();
static void accumulateOneSample();
static void finalizeAverageWindow();
static void pulseActivityLed();
static void printAverageReady();

void sensorSamplerInit();  
void sensorSamplerService();  

SensorData getAveragedSensorData();  
bool isAveragedSensorDataReady();  
void sensorSamplerAcknowledgeAverage();  

#endif  
