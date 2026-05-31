#ifndef HARDWARECONFIG_H
#define HARDWARECONFIG_H

#include <Arduino.h>

// SD card — same wiring as schematic in file SdCard.cpp
constexpr uint8_t SdCsPin = A3;
constexpr uint8_t SoftMisoPin = 12;
constexpr uint8_t SoftMosiPin = 11;
constexpr uint8_t SoftSckPin = 13;

// Pushbuttons in file DataLogger.cpp
constexpr int ManualDataLogToSDcardButton = 2;
constexpr int ClearSdButton = 3;

// ST7735 TFT in file Dashboard.cpp
constexpr uint8_t TftCsPin = 10;
constexpr uint8_t TftDcPin = 7;
constexpr uint8_t TftRstPin = 6;
constexpr uint8_t TftMosiPin = 11;
constexpr uint8_t TftSclkPin = 13;

// Logging interval (1 minute) in file DataLogger.cpp
constexpr unsigned long LogIntervalRate = 60000UL;
// Dashboard refresh interval (1 second) in file Dashboard.cpp
constexpr unsigned long DashboardRefreshRate = 1000UL;

// Sample the averaged sensor data every 2s in file SensorSampler.cpp
constexpr unsigned long SensorSamplerInMs = 2000UL;   
constexpr uint8_t NumberOfSamplePerAvg = 10;    // average of 10 samples


// External activity LED pin (you set this to D9).
constexpr uint8_t LedPin = 9;

// Visible pulse length on each 2 s sample event.
constexpr unsigned long LedPulseMs = 200UL;

#endif
