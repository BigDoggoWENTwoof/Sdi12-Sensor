#include "AvgDataLogger.h"  // init/update API for average CSV logging
#include "HardwareConfig.h"  // NumberOfSamplePerAvg (1 sample per 2 s row)
#include "SdCard.h"  // sdCardIsReady, sdCardFs, rtcFormatTimestamp
#include "SensorSampler.h"  // getAveragedSensorData, isAveragedSensorDataReady, acknowledge
#include "SensorReading.h"  // isBme280Ok, isBh1750Ok

#include <SdFat.h>  // FsFile for opening avg_datalog.csv

namespace {  // Private helpers for this file only

const char* kAvgLogFileName = "avg_datalog.csv";  // CSV path (not datalog.csv)

void appendAvgCsvHeaderIfNew() {  // Write header row once when file does not exist
  if (!sdCardIsReady()) {  // SD not mounted — skip file creation
    return;  // Exit early
  }
  SdFs& sd = sdCardFs();  // Reference to shared SD filesystem
  if (sd.exists(kAvgLogFileName)) {  // File already on card — keep existing data
    return;  // Do not overwrite header
  }

  FsFile file = sd.open(kAvgLogFileName, FILE_WRITE);  // Create or truncate new file
  if (!file) {  // open() failed
    Serial.println(F("[AvgLog] ERROR: Could not create avg_datalog.csv."));  // Report error
    return;  // Exit without header
  }
  file.println(F("Timestamp,Temperature_C,Humidity_pct,Pressure_hPa,Lux,SampleCount,Source"));  // CSV header
  file.close();  // Flush and close file
  Serial.println(F("[AvgLog] avg_datalog.csv header created."));  // Confirm on Serial
}

void printAvgRowToSerial(const char* timestamp, const SensorData& d, uint8_t sampleCount) {  // Debug print
  Serial.print(F("[AvgLog] "));  // Log prefix
  Serial.print(timestamp);  // Print timestamp string
  Serial.print(F(" "));  // Separator
  Serial.print(d.temperature, 2);  // Average temperature
  Serial.print(F("C "));  // Unit
  Serial.print(d.humidity, 1);  // Average humidity
  Serial.print(F("% "));  // Unit
  Serial.print(d.pressure, 1);  // Average pressure
  Serial.print(F("hPa "));  // Unit
  Serial.print(d.lux, 1);  // Average lux
  Serial.print(F(" lx ("));  // Unit + open paren
  Serial.print(sampleCount);  // How many 2 s samples were averaged in this row
  Serial.println(F(" samples)"));  // Close paren + newline
}

void writeAvgRow(const char* source) {  // Append one averaged row to SD or Serial
  if (!isAveragedSensorDataReady()) {  // No new average from sensorSampler yet
    return;  // Nothing to write
  }

  const SensorData d = getAveragedSensorData();  // Copy last 2 s average (not real-time)
  char timestamp[24];  // Buffer for RTC or uptime string
  rtcFormatTimestamp(timestamp, sizeof(timestamp));  // Fill timestamp from shared RTC

  if (!sdCardIsReady()) {  // SD unavailable — print only
    Serial.print(F("[AvgLog] SD unavailable — serial only: "));  // Inform user
    printAvgRowToSerial(timestamp, d, NumberOfSamplePerAvg);  // Echo row on Serial
    return;  // Skip file write
  }

  SdFs& sd = sdCardFs();  // Shared SD instance
  FsFile file = sd.open(kAvgLogFileName, FILE_WRITE);  // Open for append at end of file
  if (!file) {  // Card removed or write error
    Serial.println(F("[AvgLog] SD write failed."));  // Report failure
    printAvgRowToSerial(timestamp, d, NumberOfSamplePerAvg);  // Still show on Serial
    return;  // Exit without closing (file not open)
  }

  file.print(timestamp);  // Column 1: timestamp
  file.print(',');  // CSV delimiter
  if (isBme280Ok()) {  // Column 2: temperature (only if BME OK at write time)
    file.print(d.temperature, 2);  // Two decimal places
  }
  file.print(',');  // Delimiter
  if (isBme280Ok()) {  // Column 3: humidity
    file.print(d.humidity, 2);  // Two decimal places
  }
  file.print(',');  // Delimiter
  if (isBme280Ok()) {  // Column 4: pressure
    file.print(d.pressure, 2);  // Two decimal places
  }
  file.print(',');  // Delimiter
  if (isBh1750Ok()) {  // Column 5: lux
    file.print(d.lux, 2);  // Two decimal places
  }
  file.print(',');  // Delimiter
  file.print(NumberOfSamplePerAvg);  // Column 6: sample count (1 per 2 s row)
  file.print(',');  // Delimiter
  file.println(source);  // Column 7: source tag e.g. ISR_Avg

  file.close();  // Commit row to SD card
  printAvgRowToSerial(timestamp, d, NumberOfSamplePerAvg);  // Mirror row to Serial Monitor
}

}  // namespace

void avgDataLoggerInit() {  // Call from setup() after sdCardInit() and sensorSamplerInit()
  appendAvgCsvHeaderIfNew();  // Ensure avg_datalog.csv exists with header
  Serial.println(F("[AvgLog] Ready (avg_datalog.csv)."));  // Ready message
}

void avgDataLoggerUpdate() {  // Call each loop(); writes one row about every 2 s
  if (isAveragedSensorDataReady()) {  // New 2 s average completed by TC4 path
    writeAvgRow("ISR_Avg");  // Append row tagged as interrupt-driven average
    sensorSamplerAcknowledgeAverage();  // Clear flag so same average is not logged twice
  }
}
