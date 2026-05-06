#include <Wire.h>
#include "RTClib.h"
#include "SdFat.h"
#include <Bounce2.h>   // easy debouncing – install via Library Manager

// ===================== SD CARD (soft SPI) =====================
SdFs sd;
FsFile file;

const uint8_t SD_CS_PIN = A3;
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;

SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(4), &softSpi)

// ===================== RTC =====================
RTC_DS1307 rtc;

// ===================== LOGGING =====================
const unsigned long LOG_INTERVAL = 60000;   // 60 seconds
unsigned long lastLogTime = 0;
const char* logFileName = "datalog.csv";

// Sensors
const int sensorPins[] = {A0, A1};
const int numSensors = sizeof(sensorPins) / sizeof(sensorPins[0]);

// ===================== BUTTONS =====================
const int btnManualLog = 2;
const int btnClearSD = 3;

Bounce debouncerManual = Bounce();
Bounce debouncerClear = Bounce();

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // --- Button init (internal pull‑up) ---
  pinMode(btnManualLog, INPUT_PULLUP);
  pinMode(btnClearSD, INPUT_PULLUP);
  debouncerManual.attach(btnManualLog);
  debouncerManual.interval(50);   // debounce 50ms
  debouncerClear.attach(btnClearSD);
  debouncerClear.interval(50);

  Serial.println(F("=== RTC + SD Logger with Buttons ==="));

  // ---- RTC init ----
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println(F("ERROR: RTC not found!"));
    while (1);
  }
  Serial.println(F("RTC initialised."));
  // *** Uncomment once to set time ***
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // ---- SD init ----
  if (!sd.begin(SD_CONFIG)) {
    Serial.println(F("ERROR: SD card init failed!"));
    while (1);
  }
  Serial.println(F("SD card initialised."));

  // Create CSV header if missing
  if (!sd.exists(logFileName)) {
    file = sd.open(logFileName, FILE_WRITE);
    if (file) {
      file.print("Timestamp");
      for (int i = 0; i < numSensors; i++) {
        file.print(",Sensor");
        file.print(i + 1);
      }
      file.println();
      file.close();
      Serial.println(F("CSV header created."));
    } else {
      Serial.println(F("ERROR: Could not create CSV header."));
    }
  } else {
    Serial.println(F("CSV file exists. Appending data."));
  }

  Serial.println(F("Ready. Logging every 60s. Press button on pin3=manual log, pin4=clear SD."));
}

void loop() {
  // 1. Periodic logging every 60 seconds
  if (millis() - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = millis();
    logData("Periodic");
  }

  // 2. Check buttons (non‑blocking)
  debouncerManual.update();
  debouncerClear.update();

  if (debouncerManual.fell()) {         // button pressed (LOW)
    logData("Manual");
    Serial.println(F("Manual log triggered."));
  }

  if (debouncerClear.fell()) {
    clearSDCard();
  }

  delay(50);   // small delay for stability
}

// -------------------------------------------------------------
//  Log data (same routine for periodic and manual)
// -------------------------------------------------------------
void logData(const char* source) {
  // Get timestamp
  DateTime now = rtc.now();
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());

  // Read sensors
  int sensorValues[numSensors];
  for (int i = 0; i < numSensors; i++) {
    sensorValues[i] = analogRead(sensorPins[i]);
  }

  // Print to Serial
  Serial.print(F("Logging ("));
  Serial.print(source);
  Serial.print(F("): "));
  Serial.print(timestamp);
  for (int i = 0; i < numSensors; i++) {
    Serial.print(F(", "));
    Serial.print(sensorValues[i]);
  }
  Serial.println();

  // Write to SD card
  file = sd.open(logFileName, FILE_WRITE);
  if (file) {
    file.print(timestamp);
    for (int i = 0; i < numSensors; i++) {
      file.print(",");
      file.print(sensorValues[i]);
    }
    file.println();
    file.close();
  } else {
    Serial.println(F("ERROR: Failed to open CSV file for writing!"));
  }
}

// -------------------------------------------------------------
//  Clear SD card memory – delete the CSV file
// -------------------------------------------------------------
void clearSDCard() {
  Serial.println(F("Clearing SD card memory..."));
  if (sd.exists(logFileName)) {
    if (sd.remove(logFileName)) {
      Serial.println(F("datalog.csv deleted successfully."));
      // Recreate header (so new logs start fresh)
      file = sd.open(logFileName, FILE_WRITE);
      if (file) {
        file.print("Timestamp");
        for (int i = 0; i < numSensors; i++) {
          file.print(",Sensor");
          file.print(i + 1);
        }
        file.println();
        file.close();
        Serial.println(F("New CSV header created."));
      }
    } else {
      Serial.println(F("ERROR: Could not delete datalog.csv"));
    }
  } else {
    Serial.println(F("No file to delete."));
  }
}
