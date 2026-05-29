#include "sdi12.h"  // SDI-12 API declarations (init/handle/send/parse)
#include "SensorReading.h"  // Sensor read helpers + SensorData access

#define SDI12_BAUD 1200  // SDI-12 required baud rate
#define TX_LOCKOUT_MS 50  // Ignore RX for a short time after TX to avoid echo/noise
#define R0_STREAM_INTERVAL_MS 1000  // Continuous R0 response interval in ms
#define CMD_FRAME_TIMEOUT_MS 120  // Drop partial command if no new byte arrives in this time
#define CMD_MAX_LEN 16  // Maximum accepted command payload length (without '!')

static char sensorAddress;  // Current SDI-12 address for this sensor node
static int DIRO_PIN;  // Direction-control pin for RS485/line driver
static unsigned long txLockoutUntil = 0;  // Timestamp until RX is temporarily ignored

enum class ParserState : uint8_t {
  IDLE,  // Waiting for the first valid character
  RECEIVE,  // Collecting incoming command bytes
  COMMAND_END,  // '!' detected; command frame complete
  PROCESSING,  // Decoding command + preparing response string
  SEND_OUTPUT  // Sending response back to master (if any)
};

static ParserState parserState = ParserState::IDLE;  // Current parser FSM state
static char rxCmd[CMD_MAX_LEN + 1] = {0};  // Fixed command buffer (+1 for null terminator)
static uint8_t rxLen = 0;  // Number of bytes currently stored in rxCmd
static unsigned long lastRxByteMs = 0;  // Last time a valid RX byte was accepted
static String pendingResponse = "";  // Response prepared in PROCESSING, sent in SEND_OUTPUT

// R0 streaming state
static bool streamR0 = false;  // True while periodic R0 streaming mode is active
static unsigned long lastStreamMs = 0;  // Last timestamp when streamed R0 was sent

// ===================== TX =====================
void sdiSend(String response) {
  digitalWrite(DIRO_PIN, LOW);  // Switch transceiver to TX mode
  delay(15);  // Allow line driver to settle before writing bytes

  Serial1.print(response);  // Send already-formatted SDI-12 response
  Serial1.flush();  // Wait until UART shift register is empty

  delay(10);  // Small hold-time to ensure final bit fully exits line
  digitalWrite(DIRO_PIN, HIGH);  // Return transceiver to RX/listen mode

  while (Serial1.available()) Serial1.read();  // Drain any echoed bytes/noise left in UART RX

  txLockoutUntil = millis() + TX_LOCKOUT_MS;  // Start temporary RX lockout window

  // Debug only
  //Serial.print("[TX] ");
  //Serial.println(response);
}

// ===================== BUILDERS =====================

// Full system (R0)
String buildAllString() {
  SensorData data = getSensorData();  // Snapshot latest sensor values

  String values = String(sensorAddress);  // SDI-12 response must start with sensor address

  if (data.ready) {
    values += "+" + String(data.temperature, 2);
    values += "+" + String(data.humidity, 2);
    values += "+" + String(data.pressure, 2);
    values += "+" + String(data.lux, 2);
  } else {
    values += "+0.00+0.00+0.00+0.00";
  }

  return values;  // Return full R0 payload string
}

// BME280 only (D1)
String buildBMEString() {
  SensorData data = getSensorData();  // Snapshot latest sensor values

  String values = String(sensorAddress);  // Prefix with sensor address

  if (data.ready) {
    values += "+" + String(data.temperature, 2);
    values += "+" + String(data.humidity, 2);
    values += "+" + String(data.pressure, 2);
  } else {
    values += "+0.00+0.00+0.00";
  }

  return values;  // Return D1 payload (temperature/humidity/pressure)
}

// BH1750 only (D2)
String buildLightString() {
  SensorData data = getSensorData();  // Snapshot latest sensor values

  String values = String(sensorAddress);  // Prefix with sensor address

  if (data.ready) {
    values += "+" + String(data.lux, 2);
  } else {
    values += "+0.00";
  }

  return values;  // Return D2 payload (lux only)
}

static void resetParser() {
  rxLen = 0;  // Clear length counter
  rxCmd[0] = '\0';  // Make buffer an empty C-string
  parserState = ParserState::IDLE;  // Return FSM to waiting state
}

// ===================== COMMAND PARSER =====================
static String parseCommandInternal(const String& cmdInput) {
  //at the start of parseCommand(), streamR0 is cleared,
  // so 0M!, 0D1!, ?, address change, etc. all stop streaming before that command is handled.
  streamR0 = false;

  String cmd = cmdInput;  // Make mutable copy so trim() doesn't modify caller-owned string
  cmd.trim();  // Remove accidental spaces/newlines around command text

  // Debug only
  //Serial.print("[RX] ");
  //Serial.println(cmd);

  if (cmd.length() == 0) return "";  // Ignore empty frames

  char cmdAddr = cmd.charAt(0);  // First character is the address (or '?' query)

  if (cmd == "?") {
    return String(sensorAddress) + "\r\n";
  }

  if (cmdAddr != sensorAddress) return "";  // Ignore commands addressed to other sensors

  String body = cmd.substring(1);  // Command content after address

  // ---------- Address change ----------
  if (body.length() == 2 && body.charAt(0) == 'A') {
    char newAddr = body.charAt(1);  // Requested new address value

    if (isAlphaNumeric(newAddr)) {
      sensorAddress = newAddr;

      Serial.print("[INFO] New address: ");
      Serial.println(newAddr);

      return String(newAddr) + "\r\n";
    }
    return "";
  }

  // ---------- MEASURE ----------
  if (body == "M") {
    readSensors();

    int n = getParameterCount();  // Number of parameters that D-command can return
    String response = String(sensorAddress) + "003" + String(n);  // "ttt n" style SDI-12 measure reply

    return response + "\r\n";
  }

  // ---------- DATA BME280 ----------
  if (body == "D1") {
    readSensors();

    return buildBMEString() + "\r\n";
  }

  // ---------- DATA LIGHT ----------
  if (body == "D2") {
    readSensors();

    return buildLightString() + "\r\n";
  }

  // ---------- ALL DATA (continuous until next command) ----------
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

void parseCommand(String cmd) {
  const String response = parseCommandInternal(cmd);  // Build response using same core parser
  if (response.length() > 0) {
    sdiSend(response);  // Send only when parser produced a valid response
  }
}

static bool isValidCommandChar(char ch) {
  return isAlphaNumeric(ch) || ch == '?';  // Allowed payload chars for this command set
}

static void fsmReceiveByte(char ch) {
  switch (parserState) {
    case ParserState::IDLE:
      if (isValidCommandChar(ch)) {
        rxLen = 0;  // Start a fresh frame
        rxCmd[rxLen++] = ch;  // Save first byte
        rxCmd[rxLen] = '\0';  // Keep C-string termination valid after append
        lastRxByteMs = millis();  // Record byte time for timeout detection
        parserState = ParserState::RECEIVE;  // Move FSM into receive mode
      }
      break;  // Done handling byte in IDLE

    case ParserState::RECEIVE:
      if (ch == '!') {
        parserState = ParserState::COMMAND_END;  // End-of-frame marker received
      } else if (isValidCommandChar(ch)) {
        if (rxLen >= CMD_MAX_LEN) {
          Serial.println(F("[WARN] SDI cmd overflow, dropping frame."));  // Frame too long => invalid
          resetParser();  // Abort frame and recover parser
        } else {
          rxCmd[rxLen++] = ch;  // Append next valid byte to command buffer
          rxCmd[rxLen] = '\0';  // Keep buffer null-terminated
          lastRxByteMs = millis();  // Refresh timeout reference for active frame
        }
      } else {
        Serial.println(F("[WARN] SDI invalid char, dropping frame."));  // Corrupted character
        resetParser();  // Abort bad frame and wait for next clean one
      }
      break;  // Done handling byte in RECEIVE

    default:
      // COMMAND_END/PROCESSING/SEND_OUTPUT are advanced by loop state machine.
      break;  // Other states advance in sdi12Handle(), not here
  }
}

// ===================== INIT =====================
void sdi12Init(char address, int dirPin) {
  sensorAddress = address;  // Store initial SDI-12 sensor address
  DIRO_PIN = dirPin;  // Store transceiver direction pin

  Serial1.begin(SDI12_BAUD, SERIAL_7E1);  // SDI-12 UART settings: 1200 baud, 7E1

  pinMode(DIRO_PIN, OUTPUT);  // Configure direction pin as output
  digitalWrite(DIRO_PIN, HIGH);  // Default to listen mode (RX)

  Serial.println("[SDI-12] Initialized");  // Startup trace line
}

// ===================== LOOP HANDLER =====================
void sdi12Handle() {
  if (millis() < txLockoutUntil) return;  // Ignore RX during post-TX lockout period

  while (Serial1.available() && parserState != ParserState::SEND_OUTPUT) {
    const int c = Serial1.read();  // Read one raw UART byte

    if (c < 32 || c > 126) continue;  // Ignore non-printable/non-ASCII bytes

    fsmReceiveByte(static_cast<char>(c));  // Feed sanitized byte into FSM
  }

  if (parserState == ParserState::RECEIVE) {
    const unsigned long now = millis();  // Current time for frame-timeout check
    if (now - lastRxByteMs > CMD_FRAME_TIMEOUT_MS) {
      Serial.println(F("[WARN] SDI frame timeout, dropping partial command."));  // Incomplete frame
      resetParser();  // Clear partial command and recover
    }
  }

  if (parserState == ParserState::COMMAND_END) {
    parserState = ParserState::PROCESSING;  // Transition to parse/dispatch phase
  }

  if (parserState == ParserState::PROCESSING) {
    pendingResponse = parseCommandInternal(String(rxCmd));  // Build response for complete frame
    parserState = ParserState::SEND_OUTPUT;  // Move to TX stage (or no-op if empty)
  }

  if (parserState == ParserState::SEND_OUTPUT) {
    if (pendingResponse.length() > 0) {
      sdiSend(pendingResponse);  // Send response to SDI-12 master
    }
    pendingResponse = "";  // Clear response buffer for next frame
    resetParser();  // Return parser to IDLE for next command
  }

  // Stream R0 every R0_STREAM_INTERVAL_MS
  if (streamR0 && (millis() - lastStreamMs >= R0_STREAM_INTERVAL_MS)) {
    readSensors();  // Refresh sensor values before periodic stream send
    sdiSend(buildAllString() + "\r\n");  // Push one new R0 frame
    lastStreamMs = millis();  // Update stream timestamp for next interval
  }
}