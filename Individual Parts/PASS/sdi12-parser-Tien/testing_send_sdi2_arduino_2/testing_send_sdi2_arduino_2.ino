void setup() {
  Serial.begin(9600);
  Serial.println("Starting");

  Serial1.begin(1200, SERIAL_7E1);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH); // Start in receive mode
}

void loop() {
  // Forward Serial Monitor input to SDI-12
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.replace("\r", "");
    cmd.replace("\n", "");
    cmd.trim();
    digitalWrite(7, LOW);
    delay(15);
    Serial1.print(cmd + "\r\n");
    Serial1.flush();
    delay(10);
    digitalWrite(7, HIGH);
  }

  // Parse incoming SDI-12 commands (terminated by "!")
  if (Serial1.available()) {
    String received = Serial1.readStringUntil('!');
    for (int i = 0; i < received.length(); i++) {
  Serial.print(i);
  Serial.print(": ");
  Serial.println((int)received[i]);
}
    received.trim();

    Serial.print("Received SDI-12 command: ");
    Serial.println(received);

    // Parser logic
    if (received.startsWith("0I")) {
      Serial.println(">> Identification request");
      // respond with identification string
    } else if (received == "0M") {
      Serial.println(">> Measurement request");
      // trigger sensor read
    } else if (received == "0D0") {
      Serial.println(">> Data request");
      // send back sensor data
    } else {
      Serial.println(">> Unknown command");
    }
  }
}