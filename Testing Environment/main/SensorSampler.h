#ifndef SENSORSAMPLER_H  // Include guard start
#define SENSORSAMPLER_H  // Define guard macro

#include "SensorReading.h"  // SensorData struct and readSensors()

// TC4 ISR requests a sample every kSensorSampleMs (2 s); call sensorSamplerService() from loop().
void sensorSamplerInit();  // Start hardware timer (SAMD) or millis fallback
void sensorSamplerService();  // Process pending ISR ticks: read sensors and build averages

SensorData getAveragedSensorData();  // Last completed 2 s average (for logging only)
bool isAveragedSensorDataReady();  // True when a new average is ready to log
void sensorSamplerAcknowledgeAverage();  // Clear ready flag after AvgDataLogger writes CSV row

#endif  // Include guard end
