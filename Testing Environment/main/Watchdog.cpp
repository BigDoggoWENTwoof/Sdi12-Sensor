#include "Watchdog.h"

#if defined(ARDUINO_ARCH_AVR)
#include <avr/wdt.h>
#include <avr/io.h>
#endif

namespace {
#if defined(ARDUINO_ARCH_AVR)
constexpr uint8_t kWdtPeriod = WDTO_8S;  // Long timeout avoids false reset during heavy SD writes.
#endif
}  // namespace

void watchdogInit() {
#if defined(ARDUINO_ARCH_AVR)
  // If previous reboot was watchdog-triggered, clear the flag and disable WDT first.
  if ((MCUSR & _BV(WDRF)) != 0) {
    MCUSR &= ~_BV(WDRF);
    wdt_disable();
  }

  wdt_enable(kWdtPeriod);  // Start hardware watchdog.
  wdt_reset();  // Begin fresh timeout window immediately.
  Serial.println(F("[WDT] Enabled (8s)."));
#else
  Serial.println(F("[WDT] HW watchdog not configured for this MCU core."));
#endif
}

void watchdogKick() {
#if defined(ARDUINO_ARCH_AVR)
  wdt_reset();  // Healthy heartbeat: prevent reset.
#endif
}
