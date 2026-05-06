String cmd = "";
void setup() {
  //Arduino IDE Serial Monitor
  Serial.begin(9600);
  Serial.println("Starting");
 
  //SDI-12
  Serial1.begin(1200, SERIAL_7E1);      //SDI-12 UART, configures serial port for 7 data bits, even parity, and 1 stop bit
  pinMode(7, OUTPUT);                   //DIRO Pin
 
  //DIRO Pin LOW to Send to SDI-12
  digitalWrite(7, LOW);
  Serial1.println("HelloWorld");
  delay(100);
 
  //HIGH to Receive from SDI-12
  digitalWrite(7, HIGH);
}
 
 
void loop() {
  while (Serial1.available()) {
    int c = Serial1.read();
 
    //  Filter EVERYTHING except valid printable ASCII
    if (c < 32 || c > 126) continue;
 
    char ch = (char)c;
 
    // Build command
    cmd += ch;
 
    // SDI-12 commands end with '!'
    if (ch == '!') {
      Serial.print("Command: ");
      Serial.println(cmd);
 
      cmd = ""; // reset for next command
    }
  }
}