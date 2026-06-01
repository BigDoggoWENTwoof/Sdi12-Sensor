#include "sdi12.h"  
#include "SensorReading.h"  

#define SDI12_BAUD 1200  
#define TX_LOCKOUT_MS 50  
#define R0_STREAM_INTERVAL_MS 1000  
#define CMD_FRAME_TIMEOUT_MS 120  
#define CMD_MAX_LEN 16  

static char sensorAddress;  
static int DIRO_PIN;  
static unsigned long txLockoutUntil = 0;  

// Finite State Machine (FSM) states
enum class ParserState : uint8_t {
  IDLE,  // Waiting for the first valid character
  RECEIVE,  // Collecting incoming command bytes
  COMMAND_END,  // '!' detected; command frame complete
  PROCESSING,  // Decoding command + preparing response string
  SEND_OUTPUT  // Sending response back to master (if any)
};

// Starting state of the parser FSM
static ParserState parserState = ParserState::IDLE;  

static char rxCmd[CMD_MAX_LEN + 1] = {0}; 
static uint8_t rxLen = 0;  
static unsigned long lastRxByteMs = 0;  
static String pendingResponse = "";  

// R0 streaming state
static bool streamR0 = false;  
static unsigned long lastStreamMs = 0;  

// DIRO Pin LOW to Send Data to SDI-12 on PC, HIGH to Receive commands from SDI-12 on PC
void sdiSend(String response) {
  digitalWrite(DIRO_PIN, LOW);  
  delay(15);  

  Serial1.print(response);  
  Serial1.flush();  

  delay(10);  
  digitalWrite(DIRO_PIN, HIGH);  

  while (Serial1.available()) Serial1.read();  

  txLockoutUntil = millis() + TX_LOCKOUT_MS;  

  //Serial.print("[TX] ");
  //Serial.println(response);
}

// Builds the R0 string for both BME280 and BH1750 (4 parameters)
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

// Returns the BME280 data (temperature, humidity, pressure) with D1 command
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

// Returns the BH1750 data (lux) with D2 command
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

// Resets the parser state to IDLE by clearing the command buffer and resetting the state
static void resetParser() {
  rxLen = 0;  
  rxCmd[0] = '\0';  
  parserState = ParserState::IDLE;  
}

// Parses the command and returns the response
static String parseCommandInternal(const String& cmdInput) {
  streamR0 = false;

  String cmd = cmdInput;  
  cmd.trim();  

  if (cmd.length() == 0) return "";  

  char cmdAddr = cmd.charAt(0);  

  if (cmd == "?") {
    return String(sensorAddress) + "\r\n";
  }

  if (cmdAddr != sensorAddress) return "";  

  String body = cmd.substring(1);  

  if (body.length() == 2 && body.charAt(0) == 'A') {
    char newAddr = body.charAt(1);  

    if (isAlphaNumeric(newAddr)) {
      sensorAddress = newAddr;

      Serial.print("[INFO] New address: ");
      Serial.println(newAddr);

      return String(newAddr) + "\r\n";
    }
    return "";
  }

  if (body == "M") {
    readSensors();

    int n = getParameterCount();  
    String response = String(sensorAddress) + "003" + String(n);  

    return response + "\r\n";
  }

  if (body == "D1") {
    readSensors();

    return buildBMEString() + "\r\n";
  }

  if (body == "D2") {
    readSensors();

    return buildLightString() + "\r\n";
  }

  if (body == "R0") {
    readSensors();

    streamR0 = true;
    lastStreamMs = millis();
    return buildAllString() + "\r\n";
  }

  Serial.print("[WARN] Unknown command: ");
  Serial.println(cmd);
  return "";
}

// Receives a command and sends the response using the parseCommandInternal function above
void parseCommand(String cmd) {
  const String response = parseCommandInternal(cmd);  
  if (response.length() > 0) {
    sdiSend(response);  
  }
}

// Checks if the character is a valid command character like A, D1, D2, R0, etc.
static bool isValidCommandChar(char ch) {
  return isAlphaNumeric(ch) || ch == '?';  
}

// Finite State Machine (FSM) logic to receive a byte and process the new state + action
static void fsmReceiveByte(char ch) {
  switch (parserState) {
    case ParserState::IDLE:
      if (isValidCommandChar(ch)) {
        rxLen = 0;  
        rxCmd[rxLen] = ch;
        rxLen++;
        rxCmd[rxLen] = '\0';
        lastRxByteMs = millis();  
        parserState = ParserState::RECEIVE;  
      }
      break;  // Done handling byte in IDLE

    case ParserState::RECEIVE:
      if (ch == '!') {
        parserState = ParserState::COMMAND_END;  
      } else if (isValidCommandChar(ch)) {
        if (rxLen >= CMD_MAX_LEN) {
          Serial.println(F("[WARN] SDI cmd overflow, dropping frame."));  
          resetParser();  
        } else {
          rxCmd[rxLen] = ch;
          rxLen++;
          rxCmd[rxLen] = '\0';
          lastRxByteMs = millis();
        }
      } else {
        Serial.println(F("[WARN] SDI invalid char, dropping frame."));  
        resetParser();  
      }
      break;  

    default:
      break;  
  }
}

// Initializes the SDI-12 interface by setting the address and direction pin
void sdi12Init(char address, int dirPin) {
  sensorAddress = address;  
  DIRO_PIN = dirPin;  

  Serial1.begin(SDI12_BAUD, SERIAL_7E1);  

  pinMode(DIRO_PIN, OUTPUT);  
  digitalWrite(DIRO_PIN, HIGH);  

  Serial.println("[SDI-12] Initialized");  
}

// Main loop function that handles the SDI-12 interface (called in loop() in main.ino)
void sdi12Handle() {
  if (millis() < txLockoutUntil) return;  

  while (Serial1.available() && parserState != ParserState::SEND_OUTPUT) {
    const int c = Serial1.read();  

    if (c < 32 || c > 126) continue;  

    fsmReceiveByte(static_cast<char>(c));  
  }

  if (parserState == ParserState::RECEIVE) {
    const unsigned long now = millis();  
    if (now - lastRxByteMs > CMD_FRAME_TIMEOUT_MS) {
      Serial.println(F("[WARN] SDI frame timeout, dropping partial command."));  
      resetParser();  
    }
  }

  if (parserState == ParserState::COMMAND_END) {
    parserState = ParserState::PROCESSING;  
  }

  if (parserState == ParserState::PROCESSING) {
    pendingResponse = parseCommandInternal(String(rxCmd));  
    parserState = ParserState::SEND_OUTPUT;  
  }

  if (parserState == ParserState::SEND_OUTPUT) {
    if (pendingResponse.length() > 0) {
      sdiSend(pendingResponse);  
    }
    pendingResponse = "";  
    resetParser();  
  }

  if (streamR0 && (millis() - lastStreamMs >= R0_STREAM_INTERVAL_MS)) {
    readSensors();  
    sdiSend(buildAllString() + "\r\n");  
    lastStreamMs = millis();  
  }
}