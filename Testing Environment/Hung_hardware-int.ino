// =============================================================
//  RTC + SD Logger  ·  TC4 10 ms Timer  ·  Hardware Interrupts
//
//  Board  : Arduino Zero / MKR / SAMD21
//
//  Pin map:
//    A0  – temperature sensor A
//    A1  – temperature sensor B
//    A2  – brightness sensor A
//    A4  – brightness sensor B
//    A3  – SD card CS
//    11  – SD MOSI
//    12  – SD MISO
//    13  – SD SCK
//    2   – MANUAL LOG button
//    3   – CLEAR SD button
// =============================================================

#include <Wire.h>
#include "RTClib.h"
#include "SdFat.h"

// ================= SD CARD =================

const uint8_t SD_CS_PIN     = A3;
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;

SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(4), &softSpi)

SdFs sd;
FsFile logFile;

const char* LOG_FILE = "datalog.csv";

// ================= RTC =================

RTC_DS1307 rtc;

// ================= SENSOR PINS =================

const int TEMP_PIN_A   = A0;
const int TEMP_PIN_B   = A1;
const int BRIGHT_PIN_A = A2;
const int BRIGHT_PIN_B = A4;   // A3 is used for SD card CS, so brightness B uses A4

// ================= BUTTON PINS =================

const int BTN_MANUAL = 2;
const int BTN_CLEAR  = 3;

// ================= TIMER =================

#define TC4_PERIOD_TICKS 469   // About 10 ms using 48 MHz / 1024

// ================= FLAGS =================

volatile bool timerFlag     = false;
volatile bool manualLogFlag = false;
volatile bool clearSDFlag   = false;

// =============================================================
// Average functions
// =============================================================

float avg_temp(int a, int b) {
    return (a + b) * 0.5f;
}

float avg_bright(int a, int b) {
    return (a + b) * 0.5f;
}

// =============================================================
// Log data function
// =============================================================

void logData(const char* source) {

    float temperature = avg_temp(
        analogRead(TEMP_PIN_A),
        analogRead(TEMP_PIN_B)
    );

    float brightness = avg_bright(
        analogRead(BRIGHT_PIN_A),
        analogRead(BRIGHT_PIN_B)
    );

    DateTime now = rtc.now();

    char ts[20];

    snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

    Serial.print(F("["));
    Serial.print(source);
    Serial.print(F("] "));
    Serial.print(ts);
    Serial.print(F("  Temp="));
    Serial.print(temperature, 1);
    Serial.print(F("  Bright="));
    Serial.println(brightness, 1);

    logFile = sd.open(LOG_FILE, FILE_WRITE);

    if (logFile) {
        logFile.print(ts);
        logFile.print(',');
        logFile.print(source);
        logFile.print(',');
        logFile.print(temperature, 2);
        logFile.print(',');
        logFile.println(brightness, 2);
        logFile.close();
    } else {
        Serial.println(F("ERROR: cannot open CSV"));
    }
}

// =============================================================
// Clear SD function
// =============================================================

void clearSDCard() {

    // Stop TC4 while deleting the file
    TC4->COUNT16.CTRLA.bit.ENABLE = 0;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);

    if (logFile.isOpen()) {
        logFile.close();
    }

    Serial.println(F("Clearing SD card memory..."));

    if (sd.exists(LOG_FILE)) {

        if (sd.remove(LOG_FILE)) {
            Serial.println(F("Old datalog.csv deleted."));
        } else {
            Serial.println(F("ERROR: Could not delete datalog.csv"));
        }

    } else {
        Serial.println(F("No old file found."));
    }

    // Create a new empty CSV file with header
    logFile = sd.open(LOG_FILE, FILE_WRITE);

    if (logFile) {
        logFile.println(F("Timestamp,Source,Temp_avg,Bright_avg"));
        logFile.close();
        Serial.println(F("New empty datalog.csv created."));
    } else {
        Serial.println(F("ERROR: Could not create new CSV file."));
    }

    // Restart TC4 after clearing SD
    TC4->COUNT16.CTRLA.bit.ENABLE = 1;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
}

// =============================================================
// Interrupt functions
// =============================================================

void TC4_Handler() {

    if (TC4->COUNT16.INTFLAG.bit.OVF && TC4->COUNT16.INTENSET.bit.OVF) {
        timerFlag = true;
        TC4->COUNT16.INTFLAG.bit.OVF = 1;
    }
}

void ISR_manualLog() {
    manualLogFlag = true;
}

void ISR_clearSD() {
    clearSDFlag = true;
}

// =============================================================
// TC4 setup
// =============================================================

void setupTC4() {

    GCLK->GENDIV.reg = GCLK_GENDIV_DIV(1) |
                       GCLK_GENDIV_ID(4);

    while (GCLK->STATUS.bit.SYNCBUSY);

    GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC         |
                        GCLK_GENCTRL_GENEN       |
                        GCLK_GENCTRL_SRC_DFLL48M |
                        GCLK_GENCTRL_ID(4);

    while (GCLK->STATUS.bit.SYNCBUSY);

    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN     |
                        GCLK_CLKCTRL_GEN_GCLK4 |
                        GCLK_CLKCTRL_ID_TC4_TC5;

    while (GCLK->STATUS.bit.SYNCBUSY);

    TC4->COUNT16.CC[0].reg = TC4_PERIOD_TICKS;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);

    NVIC_SetPriority(TC4_IRQn, 0);
    NVIC_EnableIRQ(TC4_IRQn);

    TC4->COUNT16.INTFLAG.bit.OVF = 1;
    TC4->COUNT16.INTENSET.bit.OVF = 1;

    TC4->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024 |
                              TC_CTRLA_WAVEGEN_MFRQ |
                              TC_CTRLA_ENABLE;

    while (TC4->COUNT16.STATUS.bit.SYNCBUSY);
}

// =============================================================
// setup
// =============================================================

void setup() {

    Serial.begin(9600);
    while (!Serial);

    pinMode(BTN_MANUAL, INPUT_PULLUP);
    pinMode(BTN_CLEAR, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BTN_MANUAL), ISR_manualLog, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_CLEAR), ISR_clearSD, FALLING);

    Wire.begin();

    if (!rtc.begin()) {
        Serial.println(F("ERROR: RTC not found"));
        while (1);
    }

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F("RTC ready."));

    if (!sd.begin(SD_CONFIG)) {
        Serial.println(F("ERROR: SD init failed"));
        while (1);
    }

    Serial.println(F("SD ready."));

    if (!sd.exists(LOG_FILE)) {

        logFile = sd.open(LOG_FILE, FILE_WRITE);

        if (logFile) {
            logFile.println(F("Timestamp,Source,Temp_avg,Bright_avg"));
            logFile.close();
            Serial.println(F("CSV header created."));
        }

    } else {
        Serial.println(F("CSV exists. Appending data."));
    }

    setupTC4();

    Serial.println(F("TC4 running."));
    Serial.println(F("Pin 2 = manual log"));
    Serial.println(F("Pin 3 = clear SD"));
}

// =============================================================
// loop
// =============================================================

void loop() {

    if (timerFlag) {
        timerFlag = false;
        logData("Periodic");
    }

    if (manualLogFlag) {
        manualLogFlag = false;
        logData("Manual");
    }

    if (clearSDFlag) {
        clearSDFlag = false;
        clearSDCard();
    }
}