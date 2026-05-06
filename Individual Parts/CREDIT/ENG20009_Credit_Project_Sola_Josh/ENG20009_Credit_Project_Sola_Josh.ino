#include "SdFat.h"
#include "RTClib.h"

RTC_DS1307 rtc;
DateTime lastPrint;

SdFs sd;
FsFile file;

const uint8_t SD_CS_PIN = A3;
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN = 13;

SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(4), &softSpi)



String formatTime(int h, int m, int s) {
  String result = "";
  if (h < 10) result += "0";
  result += String(h) + ":";
  if (m < 10) result += "0";
  result += String(m) + ":";
  if (s < 10) result += "0";
  result += String(s);
  return result;
}

void setup() {
  Serial.begin(9600);
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  lastPrint = rtc.now();

  if (!sd.begin(SD_CONFIG)) {
    Serial.println("initialization failed!");
    sd.initErrorHalt(&Serial);
    while (1);
  }
  if (!file.open("data.csv", O_RDWR | O_CREAT | O_TRUNC)) {
    sd.errorHalt(F("open failed"));
  }
  file.close();
}

void loop() {
  DateTime now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();
  String time = formatTime(hour, minute, second); 



 //change later
  float temperature = 1;
  float humidity = 2;
  float pressure = 3;
  float light = 4;



  if (now.unixtime() - lastPrint.unixtime() >= 5) {
    Serial.println(second);
    lastPrint = now;
    logCSV(time, temperature, humidity, pressure, light);
  }

  

  delay(1000);
}


void logCSV(String time, float temperature, float humidity, float pressure, float light) {
  String row = String(time) + "," + String(temperature) + "," + String(humidity) + "," + String(pressure) + "," + String(light);

  file.open("data.csv", O_RDWR | O_APPEND);
  file.println(row);
  file.close();
}