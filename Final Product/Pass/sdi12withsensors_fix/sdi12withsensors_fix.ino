#include <BH1750.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;
BH1750 lightMeter;

#define DIRO_PIN     7
#define SDI12_BAUD   1200
#define MY_ADDRESS   '0'

unsigned long txLockoutUntil = 0;
#define TX_LOCKOUT_MS 50

struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  float lux;
  bool  ready;
} sensorBuffer;

char sensorAddress = MY_ADDRESS;

// ---- ADD THIS: persistent command buffer ----
String rxCmd = "";

void readSensors() {
  sensorBuffer.temperature = bme.readTemperature();
  sensorBuffer.humidity     = bme.readHumidity();
  sensorBuffer.pressure     = bme.readPressure() / 100.0;
  sensorBuffer.lux          = lightMeter.readLightLevel();
  sensorBuffer.ready        = true;

  Serial.println("[Sensors] Reading:");
  Serial.print("  Temp: ");     Serial.println(sensorBuffer.temperature);
  Serial.print("  Humidity: "); Serial.println(sensorBuffer.humidity);
  Serial.print("  Pressure: "); Serial.println(sensorBuffer.pressure);
  Serial.print("  Lux: ");      Serial.println(sensorBuffer.lux);
}

uint16_t calculateCRC(const String &data) {
  uint16_t crc = 0;
  for (int i = 0; i < data.length(); i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else              crc <<= 1;
    }
  }
  return crc;
}

String encodeCRC(uint16_t crc) {
  char c1 = 0x40 | (crc >> 12);
  char c2 = 0x40 | ((crc >> 6) & 0x3F);
  char c3 = 0x40 | (crc & 0x3F);
  return String(c1) + String(c2) + String(c3);
}

void sdiSend(String response) {
  digitalWrite(DIRO_PIN, LOW);
  delay(15);
  Serial1.print(response);
  Serial1.flush();
  delay(10);
  digitalWrite(DIRO_PIN, HIGH);

  while (Serial1.available()) Serial1.read();

  txLockoutUntil = millis() + TX_LOCKOUT_MS;

  Serial.print("[TX] ");
  Serial.println(response);
}

String buildValueString(char addr) {
  String values = String(addr);
  if (sensorBuffer.ready) {
    values += "+" + String(sensorBuffer.temperature, 2);
    values += "+" + String(sensorBuffer.humidity, 2);
    values += "+" + String(sensorBuffer.pressure, 2);
    values += "+" + String(sensorBuffer.lux, 2);
  } else {
    values += "+0.00+0.00+0.00+0.00";
  }
  return values;
}

void parseCommand(String cmd) {
  cmd.trim();

  Serial.print("[RX] ");
  Serial.println(cmd);

  if (cmd.length() == 0) return;

  char cmdAddr = cmd.charAt(0);

  if (cmd == "?") {
    sdiSend(String(sensorAddress) + "\r\n");
    return;
  }

  if (cmdAddr != sensorAddress) {
    Serial.println("[IGNORE] Address mismatch");
    return;
  }

  String body = cmd.substring(1);

  if (body.length() == 2 && body.charAt(0) == 'A') {
    char newAddr = body.charAt(1);
    if (isAlphaNumeric(newAddr)) {
      sensorAddress = newAddr;
      sdiSend(String(newAddr) + "\r\n");
      Serial.print("[INFO] Address changed to: ");
      Serial.println(newAddr);
    } else {
      Serial.println("[ERROR] Invalid address character");
    }
    return;
  }

  if (body == "M") {
    readSensors();
    sdiSend(String(sensorAddress) + "0005\r\n");
    return;
  }

  if (body.length() == 2 && body.charAt(0) == 'D') {
    char idx = body.charAt(1);
    if (idx == '0') {
      String values = buildValueString(sensorAddress);
      uint16_t crc = calculateCRC(values);
      sdiSend(values + encodeCRC(crc) + "\r\n");
    } else {
      sdiSend(String(sensorAddress) + "\r\n");
    }
    return;
  }

  if (body.length() == 2 && body.charAt(0) == 'R') {
    char idx = body.charAt(1);
    if (idx == '0') {
      readSensors();
      sdiSend(buildValueString(sensorAddress) + "\r\n");
    } else {
      sdiSend(String(sensorAddress) + "\r\n");
    }
    return;
  }

  Serial.print("[WARN] Unknown command: ");
  Serial.println(cmd);
}

void setup() {
  Serial.begin(9600);

  Wire.begin();
  lightMeter.begin();
  bme.begin(0x76);

  Serial.println("SDI-12 Slave Parser Ready");
  Serial.print("Sensor Address: ");
  Serial.println(sensorAddress);

  Serial1.begin(SDI12_BAUD, SERIAL_7E1);
  pinMode(DIRO_PIN, OUTPUT);
  digitalWrite(DIRO_PIN, HIGH);

  sensorBuffer.ready = false;
}

void loop() {
  if (millis() < txLockoutUntil) return;

  // ---- REPLACED: character-by-character RX with printable-ASCII filter ----
  while (Serial1.available()) {
    int c = Serial1.read();

    // Discard hidden/non-printable characters (parity artifacts, framing bytes, etc.)
    if (c < 32 || c > 126) continue;

    char ch = (char)c;
    
    // SDI-12 commands are terminated by '!'
    if (ch == '!') {
      parseCommand(rxCmd);
      rxCmd = ""; // Reset buffer for next command
    }
    else {
      rxCmd += ch;
    }
  }
}