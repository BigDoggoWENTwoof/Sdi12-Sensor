void setup() {
  Serial.begin(9600);
  Serial.println("Starting");

  Serial1.begin(1200, SERIAL_7E1);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH); // Start in receive mode
}

void loop() {
  // If PC sends something via Serial Monitor, forward it over SDI-12
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    Serial.print("Sending: ");
    Serial.println(cmd);

    // Transmit
    digitalWrite(7, LOW);
    delay(15);                    // Break signal
    Serial1.print(cmd + "\r\n");
    Serial1.flush();
    delay(10);

    // Switch back to receive
    digitalWrite(7, HIGH);
  }

  // If something comes back over SDI-12, print it to Serial Monitor
  if (Serial1.available()) {
    char c = Serial1.read();
    Serial.print(c);
  }
}