#include <Wire.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  bme.begin(0x76);
}

void loop() {
  avgTemp(0, 100);   // change this to change the timeframe

  while (true) {
    // stop the code after it finishes
  }
}

// Function to average temperature between time a and time b
void avgTemp(int a, int b) {
  float total = 0;
  int count = 0;

  while (millis() / 1000 < a) {
    // wait until start time
  }

  while (millis() / 1000 <= b) {
    float currentTemp = bme.readTemperature();

    total = total + currentTemp;
    count = count + 1;

    Serial.print("Current Temperature: ");
    Serial.print(currentTemp);
    Serial.print(" C   Average Temperature: ");
    Serial.print(total / count);
    Serial.println(" C");

    delay(1000);
  }
}