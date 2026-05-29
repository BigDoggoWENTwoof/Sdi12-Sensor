#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

// Initialize hardware watchdog timer.
// Call once near the end of setup().
void watchdogInit();

// Feed (reset) watchdog timer.
// Call regularly in loop() while firmware is healthy.
void watchdogKick();

#endif
