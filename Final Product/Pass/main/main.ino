#include "SensorReading.h"
#include "sdi12.h"

#define DIRO_PIN 7
#define MY_ADDRESS '0'

void setup() {
  Serial.begin(9600);

  sensorsInit();
  sdi12Init(MY_ADDRESS, DIRO_PIN);

  Serial.println("SDI-12 Slave Ready");
}

void loop() {
  sdi12Handle();
}