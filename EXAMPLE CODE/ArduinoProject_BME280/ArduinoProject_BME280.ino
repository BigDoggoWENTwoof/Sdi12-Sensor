#include <Wire.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 BME;
#define SeaLevel_HPA (1013.25)

void setup() {
  // put your setup code here, to run once:
  //Connect serial
  Serial.begin(9600);
  if (!Serial) {
    Serial.println("Serial failed to initialise");
  } else {
    Serial.println("Serial connected");
    Serial.println();
  }

  Wire.begin();

  //BME280 setup
  BME.begin(0x76);
  if (BME.begin() == false) {
    Serial.println("BME280 not connected\nCheck sensor wiring and I2C connection (J9)");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (BME.begin() == false) {
    Serial.println("BME280 disconnected\nCheck sensor wiring and I2C connection (J9)");
  }
  Serial.println("---***---");
  Serial.println("BME280 Data");
  Serial.print("Temperature:");
  Serial.print(BME.readTemperature());
  Serial.println(" C\n");

  Serial.print("Pressure:");
  Serial.print(BME.readPressure());
  Serial.println(" hPa\n");

  Serial.print("Approximate Altitude:");
  Serial.print(BME.readAltitude(SeaLevel_HPA));
  Serial.println(" m\n");

  Serial.print("Humidity:");
  Serial.print(BME.readHumidity());
  Serial.println(" %\n");

  //reading delay
  delay(300);
}
