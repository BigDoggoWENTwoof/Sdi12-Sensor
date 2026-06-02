#include "DataLogger.h"
#include "HardwareConfig.h"
#include "SdCard.h"
#include "SensorReading.h"

#include <Bounce2.h>
#include <SdFat.h>


namespace {

const char* logFileName = "datalog.csv";

unsigned long lastLogTimeMs = 0;

Bounce debouncerManual;
Bounce debouncerClear;

void appendCsvHeaderIfNew() {
  if (!sdCardIsReady()) {
    return;
  }
  SdFs& sd = sdCardFs();
  if (sd.exists(logFileName)) {
    return;
  }
  FsFile file = sd.open(logFileName, FILE_WRITE);
  if (!file) {
    Serial.println(F("[Log] ERROR: Could not create CSV."));
    return;
  }
  file.println(F("Timestamp,Temperature_C,Humidity_pct,Pressure_hPa,Lux,Source"));
  file.close();
  Serial.println(F("[Log] datalog.csv header created."));
}

void printLogToSerial(const char* source, const char* timestamp, const SensorData& d) {
  Serial.print(F("[Log] ("));
  Serial.print(source);
  Serial.print(F(") "));
  Serial.print(timestamp);
  Serial.print(F(" "));
  Serial.print(d.temperature, 2);
  Serial.print(F("C "));
  Serial.print(d.humidity, 1);
  Serial.print(F("% "));
  Serial.print(d.pressure, 1);
  Serial.print(F("hPa "));
  Serial.print(d.lux, 1);
  Serial.println(F(" lx"));
}

void logDataInternal(const char* source) {
  char timestamp[24];
  rtcFormatTimestamp(timestamp, sizeof(timestamp));

  readSensors();
  SensorData d = getSensorData();

  if (!sdCardIsReady()) {
    Serial.print(F("[Log] SD unavailable — serial only: "));
    printLogToSerial(source, timestamp, d);
    return;
  }

  FsFile file = sdCardFs().open(logFileName, FILE_WRITE);
  if (!file) {
    Serial.println(F("[Log] SD write failed (card removed?)."));
    printLogToSerial(source, timestamp, d);
    return;
  }

  file.print(timestamp);

  file.print(',');
  if (isBme280Ok()) {
    file.print(d.temperature, 2);
  }

  file.print(',');
  if (isBme280Ok()) {
    file.print(d.humidity, 2);
  }

  file.print(',');
  if (isBme280Ok()) {
    file.print(d.pressure, 2);
  }

  file.print(',');

  if (isBh1750Ok()) {
    file.print(d.lux, 2);
  }

  file.print(',');
  file.println(source);

  file.close();
  printLogToSerial(source, timestamp, d);
}

void clearSdLogFile() {
  if (!sdCardIsReady()) {
    Serial.println(F("[Log] SD unavailable — cannot clear."));
    return;
  }
  SdFs& sd = sdCardFs();
  Serial.println(F("[Log] Clearing datalog.csv..."));
  if (sd.exists(logFileName)) {
    if (!sd.remove(logFileName)) {
      Serial.println(F("[Log] ERROR: Could not delete file."));
      return;
    }
  }
  appendCsvHeaderIfNew();
  Serial.println(F("[Log] datalog.csv reset."));
}

}  

void dataLoggerInit() {
  pinMode(kBtnManualLogPin, INPUT_PULLUP);
  pinMode(kBtnClearSdPin, INPUT_PULLUP);

  debouncerManual.attach(kBtnManualLogPin);
  debouncerManual.interval(50);
  debouncerClear.attach(kBtnClearSdPin);
  debouncerClear.interval(50);

  appendCsvHeaderIfNew();
  lastLogTimeMs = millis();
  Serial.println(F("[Log] Buttons active (manual log / clear datalog.csv)."));
}

void dataLoggerUpdate() {
  unsigned long now = millis();
  if (now - lastLogTimeMs >= kLogIntervalMs) {
    lastLogTimeMs = now;
    logDataInternal("Periodic");
  }

  debouncerManual.update();
  debouncerClear.update();

  if (debouncerManual.fell()) {
    logDataInternal("Manual");
    if (sdCardIsReady()) {
      Serial.println(F("[Log] Manual entry saved to SD."));
    }
  }

  if (debouncerClear.fell()) {
    clearSdLogFile();
  }
}
