#include <DueTimer.h>
#define led1 11
#define led2 6
boolean toggle1 = 0;
boolean toggle2 = 0;
int count=0;

void blink() { 
  count++;
  toggle1 =1;    // flag for orange led event 
  if (count==3)  // flag for green led event
    toggle2=1;

}

void setup() {
  Serial.begin(9600);
  Serial.println("Testing Interrupt Timer Due:");   
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  // put your setup code here, to run once:
  Timer3.attachInterrupt(blink);
  Timer3.start(1000000);  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (toggle1){   // 
     digitalWrite(led1, digitalRead(led1)^1); // turn on/off orange led event
     toggle1=0;
  }

  if (toggle2){
     digitalWrite(led2, digitalRead(led2)^1); // turn on/off green led event
     toggle2=0;
     count=0;
  }
}
