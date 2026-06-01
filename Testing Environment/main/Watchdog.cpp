#include "Watchdog.h"
#include "SensorReading.h"

#define WDT_KEY (0xA5)

static uint8_t sensorFailureCount = 0;
static constexpr uint8_t MaxSensorFailures = 5;

// Enable watchdog with ~5 second timeout
void watchdogInit()
{
  WDT->WDT_MR =
      WDT_MR_WDRSTEN |     // Reset MCU on timeout
      WDT_MR_WDD(1280) |   // 5 seconds
      WDT_MR_WDV(1280);    // 5 seconds

  sensorFailureCount = 0;

  Serial.println(F("[WDT] Enabled (5 second timeout)"));
}

// Reset watchdog timer, avoiding it from triggering an arduino reset
void watchdogKick()
{
  WDT->WDT_CR =
      WDT_CR_KEY(WDT_KEY)
      | WDT_CR_WDRSTT;
}

// Check sensor status and reset watchdog if sensors are failing
void watchdogCheckSensors()
{
  const bool bmeOk = isBme280Ok();
  const bool luxOk = isBh1750Ok();

  // At least one sensor is healthy
  if (bmeOk || luxOk)
  {
    sensorFailureCount = 0;
    return;
  }

  sensorFailureCount++;

  Serial.print(F("[WDT] Consecutive sensor failures = "));
  Serial.println(sensorFailureCount);

  if (sensorFailureCount >= MaxSensorFailures)
  {
    Serial.println(F("[WDT] Sensor failure persisted."));
    Serial.println(F("[WDT] Watchdog reset pending..."));

    while (1)
    {
      // Intentionally stop execution.
      // Watchdog will reset the arduino due after ~5 seconds.
    }
  }
}