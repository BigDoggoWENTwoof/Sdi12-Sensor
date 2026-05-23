#ifndef SDCARD_H  // Include guard: prevent double inclusion of this header
#define SDCARD_H  // Define guard macro for SdCard.h

#include <Arduino.h>  // Core Arduino types (uint8_t, etc.)
#include <SdFat.h>    // SdFs filesystem class for SD logging

// Shared SD card + RTC used by DataLogger and AvgDataLogger (one physical card).
void sdCardInit();  // Initialize RTC and SD card once at startup
bool sdCardIsReady();  // True if SD card responded to begin()
bool rtcIsReady();  // True if DS1307 RTC was found on I2C
void rtcFormatTimestamp(char* out, size_t outLen);  // Fill out[] with RTC or uptime string
SdFs& sdCardFs();  // Reference to the single SdFs instance for file open/write

#endif  // End of SDCARD_H include guard
