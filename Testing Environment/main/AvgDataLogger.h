#ifndef AVGDATALOGGER_H  // Include guard start
#define AVGDATALOGGER_H  // Define guard macro

#include <Arduino.h>  // Core types used by logger API

// Writes hardware-averaged rows to avg_datalog.csv (separate from datalog.csv).
void avgDataLoggerInit();  // Create avg_datalog.csv header if file is new
void avgDataLoggerUpdate();  // Append one row when sensorSampler reports new average

#endif  // Include guard end
