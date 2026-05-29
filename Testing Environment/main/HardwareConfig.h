#ifndef HARDWARECONFIG_H
#define HARDWARECONFIG_H

#include <Arduino.h>

// SD card (soft SPI) — same wiring as project.ino
constexpr uint8_t kSdCsPin = A3;
constexpr uint8_t kSoftMisoPin = 12;
constexpr uint8_t kSoftMosiPin = 11;
constexpr uint8_t kSoftSckPin = 13;

// Pushbuttons: active LOW with INPUT_PULLUP
constexpr int kBtnManualLogPin = 2;
constexpr int kBtnClearSdPin = 3;

// ST7735 TFT (hardware SPI — shares MOSI/SCK with SD soft SPI)
constexpr uint8_t kTftCsPin = 10;
constexpr uint8_t kTftDcPin = 7;
constexpr uint8_t kTftRstPin = 6;
constexpr uint8_t kTftMosiPin = 11;
constexpr uint8_t kTftSclkPin = 13;

constexpr unsigned long kLogIntervalMs = 60000UL;
constexpr unsigned long kDashboardRefreshMs = 1000UL;

// Arduino Due hardware timer: one I2C sample request every 2 s.
constexpr unsigned long kSensorSampleMs = 2000UL;
// Average window: one sample every 2 s → one averaged row every 2 s.
constexpr unsigned long kSensorAverageMs = 2000UL;
constexpr uint8_t kSamplesPerAverageWindow =
    static_cast<uint8_t>(kSensorAverageMs / kSensorSampleMs);
// HW timer ISR base tick (10 ms); ISR divides down to kSensorSampleMs (2 s).
constexpr unsigned long kHwTimerBasePeriodMs = 10UL;
// External activity LED pin (you set this to D9).
constexpr uint8_t kIsrActivityLedPin = 9;
// Set false if your external LED is wired active-low.
constexpr bool kIsrActivityLedActiveHigh = true;
// Visible pulse length on each 2 s sample event.
constexpr unsigned long kIsrActivityLedPulseMs = 200UL;

#endif
