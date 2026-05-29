//Example - Exernal interrupt

#define LED 11
#define BUTTON_PIN 2
void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600); // setup serial
  pinMode(LED, OUTPUT); // setup output
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN,displaymic,RISING); //start the int.0   // opton 1 to configure hardware/external interupt
 // attachInterrupt(digitalPinToInterrupt(21), displaymic,RISING); // option 2 

}

//ISR - external interupt
void displaymic(){
  Serial.write("micros = ");
  Serial.println(micros());
}

void loop() {
  // put your main code here, to run repeatedly:
  // turn on/off LED
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  delay(500);

}
