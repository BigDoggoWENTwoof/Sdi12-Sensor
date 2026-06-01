#include "Dashboard.h"
#include "HardwareConfig.h"
#include "SensorReading.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ============================================================
//  PRIVATE VARIABLES / FUNCTIONS
// ============================================================
namespace {
Adafruit_ST7735 tft(
  TftCsPin,
  TftDcPin,
  TftMosiPin,
  TftSclkPin,
  TftRstPin
);

unsigned long lastRefreshMs = 0;

} 

// ============================================================
//  INITIALIZE DASHBOARD
// ============================================================
void dashboardInit() {

  tft.initR(INITR_BLACKTAB);

  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);

 
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(10, 25);
  tft.print(F("Temp:"));


  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 50);
  tft.print(F("Hum:"));


  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(10, 75);
  tft.print(F("Press:"));


  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 100);
  tft.print(F("Light:"));

  lastRefreshMs = 0;
}


// ============================================================
//  UPDATE DASHBOARD VALUES
// ============================================================
void dashboardUpdate() {

  unsigned long now = millis();

  if (now - lastRefreshMs < kDashboardRefreshMs) {
    return;
  }

  lastRefreshMs = now;

  readSensors();
  SensorData d = getSensorData();

  if (!d.ready) {

    tft.fillRect(10, 120, 150, 20, ST77XX_BLACK);

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 120);
    tft.print(F("Sensors..."));

    return;
  }

  // ==========================================================
  // TEMPERATURE
  // ==========================================================


  tft.fillRect(90, 20, 70, 20, ST77XX_BLACK);

  tft.setTextColor(ST77XX_RED);
  tft.setCursor(90, 25);

  if (isBme280Ok()) {
    tft.print(d.temperature, 2);
  } else {
    tft.print(F("---"));
  }


  // ==========================================================
  // HUMIDITY
  // ==========================================================

  tft.fillRect(90, 45, 70, 20, ST77XX_BLACK);

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(90, 50);

  if (isBme280Ok()) {
    tft.print(d.humidity, 2);
  } else {
    tft.print(F("---"));
  }


  // ==========================================================
  // PRESSURE
  // ==========================================================

  tft.fillRect(90, 70, 70, 20, ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(90, 75);

  if (isBme280Ok()) {
    tft.print(d.pressure, 0);
  } else {
    tft.print(F("---"));
  }


  // ==========================================================
  // LIGHT LEVEL
  // ==========================================================

  tft.fillRect(90, 95, 70, 20, ST77XX_BLACK);

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(90, 100);

  if (isBh1750Ok()) {
    tft.print(d.lux, 2);
  } else {
    tft.print(F("---"));
  }
}