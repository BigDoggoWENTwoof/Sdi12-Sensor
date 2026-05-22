#include "sdi12.h"
#include "SensorReading.h"

#define SDI12_BAUD 1200
#define TX_LOCKOUT_MS 50
#define R0_STREAM_INTERVAL_MS 1000

static char sensorAddress;
static int DIRO_PIN;
static unsigned long txLockoutUntil = 0;

static String rxCmd = "";

// R0 streaming state
static bool streamR0 = false;
static unsigned long lastStreamMs = 0;

// ===================== TX =====================
void sdiSend(String response) {
  digitalWrite(DIRO_PIN, LOW);
  delay(15);

  Serial1.print(response);
  Serial1.flush();

  delay(10);
  digitalWrite(DIRO_PIN, HIGH);

  while (Serial1.available()) Serial1.read();

  txLockoutUntil = millis() + TX_LOCKOUT_MS;

  // Debug only
  //Serial.print("[TX] ");
  //Serial.println(response);
}

// ===================== BUILDERS =====================

// Full system (R0)
String buildAllString() {
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

// BME280 only (D1)
String buildBMEString() {
  SensorData data = getSensorData();

  String values = String(sensorAddress);

  if (data.ready) {
    values += "+" + String(data.temperature, 2);
    values += "+" + String(data.humidity, 2);
    values += "+" + String(data.pressure, 2);
  } else {
    values += "+0.00+0.00+0.00";
  }

  return values;
}

// BH1750 only (D2)
String buildLightString() {
  SensorData data = getSensorData();

  String values = String(sensorAddress);

  if (data.ready) {
    values += "+" + String(data.lux, 2);
  } else {
    values += "+0.00";
  }

  return values;
}

// ===================== COMMAND PARSER =====================
void parseCommand(String cmd) {
  //at the start of parseCommand(), streamR0 is cleared,
  // so 0M!, 0D1!, ?, address change, etc. all stop streaming before that command is handled.
  streamR0 = false;

  cmd.trim();

  // Debug only
  //Serial.print("[RX] ");
  //Serial.println(cmd);

  if (cmd.length() == 0) return;

  char cmdAddr = cmd.charAt(0);

  if (cmd == "?") {
    sdiSend(String(sensorAddress) + "\r\n");
    return;
  }

  if (cmdAddr != sensorAddress) return;

  String body = cmd.substring(1);

  // ---------- Address change ----------
  if (body.length() == 2 && body.charAt(0) == 'A') {
    char newAddr = body.charAt(1);

    if (isAlphaNumeric(newAddr)) {
      sensorAddress = newAddr;

      Serial.print("[INFO] New address: ");
      Serial.println(newAddr);

      sdiSend(String(newAddr) + "\r\n");
    }
    return;
  }

  // ---------- MEASURE ----------
  if (body == "M") {
    readSensors();

    int n = getParameterCount();
    String response = String(sensorAddress) + "003" + String(n);

    sdiSend(response + "\r\n");
    return;
  }

  // ---------- DATA BME280 ----------
  if (body == "D1") {
    readSensors();

    sdiSend(buildBMEString() + "\r\n");
    return;
  }

  // ---------- DATA LIGHT ----------
  if (body == "D2") {
    readSensors();

    sdiSend(buildLightString() + "\r\n");
    return;
  }

  // ---------- ALL DATA (continuous until next command) ----------
  if (body == "R0") {
    readSensors();
    sdiSend(buildAllString() + "\r\n");

    streamR0 = true;
    lastStreamMs = millis();
    return;
  }

  Serial.print("[WARN] Unknown command: ");
  Serial.println(cmd);
}

// ===================== INIT =====================
void sdi12Init(char address, int dirPin) {
  sensorAddress = address;
  DIRO_PIN = dirPin;

  Serial1.begin(SDI12_BAUD, SERIAL_7E1);

  pinMode(DIRO_PIN, OUTPUT);
  digitalWrite(DIRO_PIN, HIGH);

  Serial.println("[SDI-12] Initialized");
}

// ===================== LOOP HANDLER =====================
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

  // Stream R0 every R0_STREAM_INTERVAL_MS
  if (streamR0 && (millis() - lastStreamMs >= R0_STREAM_INTERVAL_MS)) {
    readSensors();
    sdiSend(buildAllString() + "\r\n");
    lastStreamMs = millis();
  }
}