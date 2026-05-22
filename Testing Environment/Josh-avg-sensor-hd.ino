    #include <Wire.h>
    #include <Adafruit_BME280.h>

    Adafruit_BME280 bme;

    void setup() {
      Serial.begin(9600);
      Wire.begin();
      bme.begin(0x76);
    }

    void loop() {
      float temp = bme.readTemperature();

      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.println(" C");

      if (sensorConnected() == true) {
        Serial.println("BME280 sensor is working");
      } else {
        Serial.println("BME280 sensor is NOT working");
      }

      Serial.println("---");

      delay(1000);
    }

    // checks if BME280 is detected on I2C address 0x76
    bool sensorConnected() {
      Wire.beginTransmission(0x76);

      if (Wire.endTransmission() == 0) {
        return true;
      } else {
        return false;
      }
    } 

﻿