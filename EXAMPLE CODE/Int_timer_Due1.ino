#include <DueTimer.h>

int LED = 11;
int state = LOW;

void blink() {
  state = !state;
  digitalWrite(LED, state);
  Serial.println("true");  
}


void setup() {
  Serial.begin(9600);
  Serial.println("Testing Interrupt Timer Due:");   
  pinMode(11, OUTPUT); // setup output  

  // put your setup code here, to run once:
  Timer3.attachInterrupt(blink);
  Timer3.start(500000);  
}

void loop() {
  // put your main code here, to run repeatedly:

}
