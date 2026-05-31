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

// Create TFT object
Adafruit_ST7735 tft(
  TftCsPin,
  TftDcPin,
  TftMosiPin,
  TftSclkPin,
  TftRstPin
);

// Store last screen refresh time
unsigned long lastRefreshMs = 0;

} // namespace


// ============================================================
//  INITIALIZE DASHBOARD
// ============================================================
void dashboardInit() {

  // Initialize TFT display
  tft.initR(INITR_BLACKTAB);

  // Rotate display to landscape
  tft.setRotation(1);

  // Clear screen once at startup
  tft.fillScreen(ST77XX_BLACK);

  // Text size
  tft.setTextSize(2);

  // ----------------------------------------------------------
  // Draw static sensor labels
  // These are only drawn ONCE
  // ----------------------------------------------------------

  // Temperature label
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(10, 25);
  tft.print(F("Temp:"));

  // Humidity label
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 50);
  tft.print(F("Hum:"));

  // Pressure label
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(10, 75);
  tft.print(F("Press:"));

  // Light label
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 100);
  tft.print(F("Light:"));

  // Reset timer
  lastRefreshMs = 0;
}


// ============================================================
//  UPDATE DASHBOARD VALUES
// ============================================================
void dashboardUpdate() {

  // Current system time
  unsigned long now = millis();

  // ----------------------------------------------------------
  // Refresh rate control
  // Prevents TFT from updating too fast
  // ----------------------------------------------------------
  if (now - lastRefreshMs < DashboardRefreshRate) {
    return;
  }

  // Save refresh time
  lastRefreshMs = now;

  // Real-time display: fresh I2C read (not ISR-averaged data used for avg_datalog.csv).
  readSensors();
  SensorData d = getSensorData();

  // ----------------------------------------------------------
  // If sensors not ready
  // ----------------------------------------------------------
  if (!d.ready) {

    // Clear message area only
    tft.fillRect(10, 120, 150, 20, ST77XX_BLACK);

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 120);
    tft.print(F("Sensors..."));

    return;
  }

  // ==========================================================
  // TEMPERATURE
  // ==========================================================

  // Clear old value area
  tft.fillRect(90, 20, 70, 20, ST77XX_BLACK);

  // Draw new value
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