#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <Arduino.h>

// Initialize TFT dashboard (call once in setup)
void dashboardInit();

// Update values on screen (call in loop)
void dashboardUpdate();

#endif