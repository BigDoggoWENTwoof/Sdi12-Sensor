#include "SdCard.h"
#include "HardwareConfig.h"

#include <RTClib.h>
#include <SdFat.h>

// Shared SD card and RTC objects used by the project
namespace {

SdFs sd;
SoftSpiDriver<kSoftMisoPin, kSoftMosiPin, kSoftSckPin> softSpi;

#define SD_CONFIG SdSpiConfig(kSdCsPin, SHARED_SPI, SD_SCK_MHZ(4), &softSpi)

RTC_DS1307 rtc;

bool sdOk = false;
bool rtcOk = false;

}

// Initialise the RTC and SD card before the loggers run
void sdCardInit() {
  rtcOk = rtc.begin();

  if (!rtcOk) {
    Serial.println(F("[SD] WARN: RTC not found — timestamps use uptime."));
  } else if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F("[SD] RTC stopped — time set from compile time."));
  }

  sdOk = sd.begin(SD_CONFIG);

  if (!sdOk) {
    Serial.println(F("[SD] WARN: SD card init failed."));
  } else {
    Serial.println(F("[SD] Card ready (shared by loggers)."));
  }
}

// Return whether the SD card is ready for writing
bool sdCardIsReady() {
  return sdOk;
}

// Return whether the RTC is available for real timestamps
bool rtcIsReady() {
  return rtcOk;
}

// Create a timestamp using RTC time, or uptime if RTC is unavailable
void rtcFormatTimestamp(char* out, size_t outLen) {
  if (rtcOk) {
    DateTime now = rtc.now();

    snprintf(out, outLen, "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
  } else {
    unsigned long sec = millis() / 1000UL;

    snprintf(out, outLen, "uptime_%lu", sec);
  }
}

// Give other files access to the shared SD filesystem object
SdFs& sdCardFs() {
  return sd;
}