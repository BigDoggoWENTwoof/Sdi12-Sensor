#include "sdi12.h"
#include "SensorReading.h"

#define SDI12_BAUD 1200
#define TX_LOCKOUT_MS 50

static char sensorAddress;
static int DIRO_PIN;
static unsigned long txLockoutUntil = 0;

static String rxCmd = "";

// ---------- CRC ----------
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

// ---------- TX ----------
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

// ---------- Build Data ----------
String buildValueString() {
  SensorData data = getSensorData();

  String values = String(sensorAddress);
  if (data.ready) {
    values += "+" + String(data.temperature, 2);
    values += "+" + String(data.humidity, 2);
    values += "+" + String(data.pressure, 2);
    values += "+" + String(data.lux, 2);
  } else {
    values += "+0.00+0.00+0.00+0.00";
  }
  return values;
}

// ---------- Command Parser ----------
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

  if (cmdAddr != sensorAddress) return;

  String body = cmd.substring(1);

  if (body.length() == 2 && body.charAt(0) == 'A') {
    char newAddr = body.charAt(1);
    if (isAlphaNumeric(newAddr)) {
      sensorAddress = newAddr;
      sdiSend(String(newAddr) + "\r\n");
    }
    return;
  }

  if (body == "M") {
    readSensors();
    sdiSend(String(sensorAddress) + "0005\r\n");
    return;
  }

  if (body == "D0") {
    String values = buildValueString();
    uint16_t crc = calculateCRC(values);
    sdiSend(values + encodeCRC(crc) + "\r\n");
    return;
  }

  if (body == "R0") {
    readSensors();
    sdiSend(buildValueString() + "\r\n");
    return;
  }
}

// ---------- Public ----------
void sdi12Init(char address, int dirPin) {
  sensorAddress = address;
  DIRO_PIN = dirPin;

  Serial1.begin(SDI12_BAUD, SERIAL_7E1);
  pinMode(DIRO_PIN, OUTPUT);
  digitalWrite(DIRO_PIN, HIGH);
}

void sdi12Handle() {
  if (millis() < txLockoutUntil) return;

  while (Serial1.available()) {
    int c = Serial1.read();

    if (c < 32 || c > 126) continue;

    char ch = (char)c;

    if (ch == '!') {
      parseCommand(rxCmd);
      rxCmd = "";
    } else {
      rxCmd += ch;
    }
  }
}