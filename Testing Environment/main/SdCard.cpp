#include "SdCard.h"
#include "HardwareConfig.h"

#include <RTClib.h>
#include <SdFat.h>

// Private SD card and RTC objects used only in this file
namespace {

SdFs sd;  // One filesystem object for the whole project
SoftSpiDriver<SoftMisoPin, SoftMosiPin, SoftSckPin> softSpi;  // Bit-banged SPI for SD
#define SD_CONFIG SdSpiConfig(SdCsPin, SHARED_SPI, SD_SCK_MHZ(4), &softSpi)  // SD chip select + 4 MHz


RTC_DS1307 rtc;

bool sdOk = false;
bool rtcOk = false;

}

// Initialise the RTC and SD card
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

// Check if the SD card is ready
bool sdCardIsReady() {
  return sdOk;
}

// Check if the RTC is ready
bool rtcIsReady() {
  return rtcOk;
}

// Create a timestamp using RTC time or uptime
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

// Give other files access to the shared SD card object
SdFs& sdCardFs() {
  return sd;
}