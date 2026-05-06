// =============================================
// SDI-12 Slave Command Parser
// Arduino Due - UART1 (Serial1)
// DIRO Pin: 7
// =============================================

// --- Config ---
#define DIRO_PIN     7
#define SDI12_BAUD   1200
#define MY_ADDRESS   '0'        // Default sensor address

//
unsigned long txLockoutUntil = 0;
#define TX_LOCKOUT_MS 50  // Ignore RX for 50ms after transmitting

// --- Sensor Data Buffer ---
struct SensorData {
  float temperature;  // °C
  float humidity;     // %
  float pressure;     // hPa
  float gas;          // kOhms
  float lux;          // lx
  bool  ready;        // Has aM! been called?
} sensorBuffer;

char sensorAddress = MY_ADDRESS;

// =============================================
// PLACEHOLDER: Simulate sensor readings
// Replace with real BME680 / BH1750 reads later
// =============================================
void readSensors() {
  // Random plausible values
  sensorBuffer.temperature = 20.0 + random(0, 100) / 10.0;   // 20.0 - 30.0 °C
  sensorBuffer.humidity     = 40.0 + random(0, 400) / 10.0;  // 40.0 - 80.0 %
  sensorBuffer.pressure     = 1000.0 + random(0, 300) / 10.0;// 1000.0 - 1030.0 hPa
  sensorBuffer.gas          = 50.0 + random(0, 500) / 10.0;  // 50.0 - 100.0 kOhms
  sensorBuffer.lux          = random(100, 10000) / 10.0;      // 10.0 - 1000.0 lx
  sensorBuffer.ready        = true;

  Serial.println("[Sensors] Reading simulated:");
  Serial.print("  Temp: ");     Serial.println(sensorBuffer.temperature);
  Serial.print("  Humidity: "); Serial.println(sensorBuffer.humidity);
  Serial.print("  Pressure: "); Serial.println(sensorBuffer.pressure);
  Serial.print("  Gas: ");      Serial.println(sensorBuffer.gas);
  Serial.print("  Lux: ");      Serial.println(sensorBuffer.lux);
}

// =============================================
// CRC Calculation (SDI-12 standard CRC-16)
// =============================================
uint16_t calculateCRC(const String &data) {
  uint16_t crc = 0;
  for (int i = 0; i < data.length(); i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }
  return crc;
}

// Encode CRC into 3 printable ASCII chars as per SDI-12 spec
String encodeCRC(uint16_t crc) {
  char c1 = 0x40 | (crc >> 12);
  char c2 = 0x40 | ((crc >> 6) & 0x3F);
  char c3 = 0x40 | (crc & 0x3F);
  return String(c1) + String(c2) + String(c3);
}

// =============================================
// SDI-12 Transmit Helper
// =============================================
void sdiSend(String response) {
  digitalWrite(DIRO_PIN, LOW);
  delay(15);
  Serial1.print(response);
  Serial1.flush();
  delay(10);
  digitalWrite(DIRO_PIN, HIGH);

  // Flush anything already in the RX buffer (the echo)
  while (Serial1.available()) Serial1.read();

  // Set lockout window
  txLockoutUntil = millis() + TX_LOCKOUT_MS;

  Serial.print("[TX] ");
  Serial.println(response);
}

// =============================================
// Build sensor value string for aD0! / aR0!
// Format: a+val1+val2+val3+val4+val5
// =============================================
String buildValueString(char addr) {
  String values = String(addr);
  if (sensorBuffer.ready) {
    values += "+" + String(sensorBuffer.temperature, 2);
    values += "+" + String(sensorBuffer.humidity, 2);
    values += "+" + String(sensorBuffer.pressure, 2);
    values += "+" + String(sensorBuffer.gas, 2);
    values += "+" + String(sensorBuffer.lux, 2);
  } else {
    values += "+0.00+0.00+0.00+0.00+0.00"; // Not yet measured
  }
  return values;
}

// =============================================
// Command Parser
// =============================================
void parseCommand(String cmd) {
  cmd.trim();

  Serial.print("[RX] ");
  Serial.println(cmd);

  // Must be at least 1 char
  if (cmd.length() == 0) return;

  char cmdAddr = cmd.charAt(0);

  // -----------------------------------------------
  // Address Query: ?!
  // Any sensor responds with its address
  // -----------------------------------------------
  if (cmd == "?") {
    String response = String(sensorAddress) + "\r\n";
    sdiSend(response);
    return;
  }

  // All other commands must match our address
  if (cmdAddr != sensorAddress) {
    Serial.println("[IGNORE] Address mismatch");
    return;
  }

  String body = cmd.substring(1); // Strip address

  // -----------------------------------------------
  // Change Address: aAb!
  // Response: b<CR><LF>
  // -----------------------------------------------
  if (body.length() == 2 && body.charAt(0) == 'A') {
    char newAddr = body.charAt(1);
    // Valid addresses: 0-9, a-z, A-Z
    if (isAlphaNumeric(newAddr)) {
      sensorAddress = newAddr;
      String response = String(newAddr) + "\r\n";
      sdiSend(response);
      Serial.print("[INFO] Address changed to: ");
      Serial.println(newAddr);
    } else {
      Serial.println("[ERROR] Invalid address character");
    }
    return;
  }

  // -----------------------------------------------
  // Start Measurement: aM!
  // Response: atttn<CR><LF>
  //   ttt = time to complete (seconds, 000-999)
  //   n   = number of values to return
  // -----------------------------------------------
  if (body == "M") {
    readSensors();
    // ttt=000 means data ready immediately, n=5 values
    String response = String(sensorAddress) + "0005\r\n";
    sdiSend(response);
    return;
  }

  // -----------------------------------------------
  // Send Data: aD0! ... aD9!
  // Response: a<values><CR><LF>
  //        or a<values><CRC><CR><LF>
  // Only D0 used here (all 5 values fit in one frame)
  // Use "0D0! input"
  // -----------------------------------------------
  if (body.length() == 2 && body.charAt(0) == 'D') {
    char idx = body.charAt(1);
    if (idx == '0') {
      String values = buildValueString(sensorAddress);
      uint16_t crc = calculateCRC(values);
      String response = values + encodeCRC(crc) + "\r\n";
      sdiSend(response);
    } else {
      // D1-D9: no more data
      String response = String(sensorAddress) + "\r\n";
      sdiSend(response);
    }
    return;
  }

  // -----------------------------------------------
  // Continuous Measurement: aR0! ... aR9!
  // Response: a<values><CR><LF> (no CRC required)
  // Reads sensors fresh each time (no aM! needed)
  // -----------------------------------------------
  if (body.length() == 2 && body.charAt(0) == 'R') {
    char idx = body.charAt(1);
    if (idx == '0') {
      readSensors(); // Fresh read every time for continuous
      String values = buildValueString(sensorAddress);
      String response = values + "\r\n";
      sdiSend(response);
    } else {
      String response = String(sensorAddress) + "\r\n";
      sdiSend(response);
    }
    return;
  }

  // -----------------------------------------------
  // Unknown Command
  // -----------------------------------------------
  Serial.print("[WARN] Unknown command: ");
  Serial.println(cmd);
}

// =============================================
// Setup
// =============================================
void setup() {
  Serial.begin(9600);
  Serial.println("SDI-12 Slave Parser Ready");
  Serial.print("Sensor Address: ");
  Serial.println(sensorAddress);

  Serial1.begin(SDI12_BAUD, SERIAL_7E1);
  pinMode(DIRO_PIN, OUTPUT);
  digitalWrite(DIRO_PIN, HIGH); // Start in receive mode

  randomSeed(analogRead(0)); // Seed RNG for placeholder sensor values
  sensorBuffer.ready = false;
}

// =============================================
// Loop
// =============================================
void loop() {
  // Skip RX during lockout window
  if (millis() < txLockoutUntil) return;

  // Poll SDI-12 bus for incoming commands
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('!');
    parseCommand(cmd);
  }
}