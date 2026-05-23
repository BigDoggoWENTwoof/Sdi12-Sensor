#include "SdCard.h"  // Public API for shared SD and RTC access
#include "HardwareConfig.h"  // Pin constants (kSdCsPin, soft SPI pins)

#include <RTClib.h>  // DS1307 RTC driver
#include <SdFat.h>   // SD card filesystem

namespace {  // File-local symbols: not visible outside this .cpp

SdFs sd;  // One filesystem object for the whole project
SoftSpiDriver<kSoftMisoPin, kSoftMosiPin, kSoftSckPin> softSpi;  // Bit-banged SPI for SD
#define SD_CONFIG SdSpiConfig(kSdCsPin, SHARED_SPI, SD_SCK_MHZ(4), &softSpi)  // SD chip select + 4 MHz

RTC_DS1307 rtc;  // Real-time clock on I2C (Wire)

bool sdOk = false;  // Cached result of sd.begin()
bool rtcOk = false;  // Cached result of rtc.begin()

}  // namespace

void sdCardInit() {  // Called once from setup() before any logger runs
  rtcOk = rtc.begin();  // Probe RTC on I2C bus
  if (!rtcOk) {  // RTC missing or wiring fault
    Serial.println(F("[SD] WARN: RTC not found — timestamps use uptime."));  // Inform user on Serial
  } else if (!rtc.isrunning()) {  // RTC found but oscillator was stopped
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set RTC from compile date/time
    Serial.println(F("[SD] RTC stopped — time set from compile time."));  // Inform user
  }

  sdOk = sd.begin(SD_CONFIG);  // Mount SD card over soft SPI
  if (!sdOk) {  // Card missing, bad wiring, or wrong format
    Serial.println(F("[SD] WARN: SD card init failed."));  // Loggers will run without SD
  } else {  // Card mounted successfully
    Serial.println(F("[SD] Card ready (shared by loggers)."));  // Confirm shared resource
  }
}

bool sdCardIsReady() {  // Query whether SD writes are allowed
  return sdOk;  // Return cached init flag
}

bool rtcIsReady() {  // Query whether RTC timestamps are available
  return rtcOk;  // Return cached RTC init flag
}

void rtcFormatTimestamp(char* out, size_t outLen) {  // Build a CSV-safe timestamp string
  if (rtcOk) {  // Use wall-clock time when RTC works
    DateTime now = rtc.now();  // Read current RTC registers
    snprintf(out, outLen, "%04d-%02d-%02d %02d:%02d:%02d",  // Format: YYYY-MM-DD HH:MM:SS
             now.year(), now.month(), now.day(),  // Date fields
             now.hour(), now.minute(), now.second());  // Time fields
  } else {  // Fallback when no RTC
    unsigned long sec = millis() / 1000UL;  // Seconds since boot
    snprintf(out, outLen, "uptime_%lu", sec);  // e.g. uptime_12345
  }
}

SdFs& sdCardFs() {  // Expose SD object to DataLogger and AvgDataLogger
  return sd;  // Return reference to the single SdFs instance
}
