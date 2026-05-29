#ifndef SENSORSAMPLER_H  // Include guard start
#define SENSORSAMPLER_H  // Define guard macro

#include "SensorReading.h"  // SensorData struct and readSensors()

// HW timer ISR requests a sample every kSensorSampleMs (2 s); call sensorSamplerService() from loop().
extern volatile uint32_t g_hwTimerIsrCount;  // Increments each ~10 ms ISR (0 = timer not firing)

void sensorSamplerInit();  // Start Arduino Due hardware timer (SAM3X TC)
void sensorSamplerService();  // Process pending ISR ticks: read sensors and build averages

SensorData getAveragedSensorData();  // Last completed 2 s average (for logging only)
bool isAveragedSensorDataReady();  // True when a new average is ready to log
void sensorSamplerAcknowledgeAverage();  // Clear ready flag after AvgDataLogger writes CSV row

#endif  // Include guard end
