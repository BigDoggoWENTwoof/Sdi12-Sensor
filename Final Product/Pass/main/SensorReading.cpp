#include "SensorReading.h"
#include <Wire.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

bool bmeOK = false;
bool lightOK = false;

static SensorData sensorBuffer;

void sensorsInit() {
  Wire.begin();

  // Initialize sensors and capture status
  lightOK = lightMeter.begin();
  bmeOK   = bme.begin(0x76);

  Serial.print("[Init] BME280: ");
  Serial.println(bmeOK ? "OK" : "FAIL");

  Serial.print("[Init] BH1750: ");
  Serial.println(lightOK ? "OK" : "FAIL");

  sensorBuffer.ready = false;
}

void readSensors() {
  // ---- BME280 ----
  float pres = bme.readPressure();

  if (pres < 0) {
    bmeOK = false;
  } else {
    bmeOK = true;
    sensorBuffer.temperature = bme.readTemperature();
    sensorBuffer.humidity    = bme.readHumidity();
    sensorBuffer.pressure    = pres / 100.0;
  }

  // ---- BH1750 ----
  float lux = lightMeter.readLightLevel();

  if (lux < 0) {   // error condition
    lightOK = false;
  } else {
    lightOK = true;
    sensorBuffer.lux = lux;
  }

  // ---- Ready flag ----
  sensorBuffer.ready = (bmeOK || lightOK);

  // ---- Debug prints (Remove at the END) ----
  /*
  Serial.println("[Sensors] Reading:");
  Serial.print("  Temp: ");     Serial.println(sensorBuffer.temperature);
  Serial.print("  Humidity: "); Serial.println(sensorBuffer.humidity);
  Serial.print("  Pressure: "); Serial.println(sensorBuffer.pressure);
  Serial.print("  Lux: ");      Serial.println(sensorBuffer.lux);
  Serial.print("  BME OK: ");   Serial.println(bmeOK ? "YES" : "NO");
  Serial.print("  Light OK: "); Serial.println(lightOK ? "YES" : "NO");
  */


}


SensorData getSensorData() {
  return sensorBuffer;
}

int getParameterCount() {
  int count = 0;

  if (bmeOK) {
    count += 3; // temperature, humidity, pressure
  }

  if (lightOK) {
    count += 1; // lux
  }

  return count;
}