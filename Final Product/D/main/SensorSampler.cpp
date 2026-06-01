#include "SensorSampler.h"  // Public sampler API declarations
#include "HardwareConfig.h"  // kSensorSampleMs, kSensorAverageMs, kSamplesPerAverageWindow

#if defined(ARDUINO_ARCH_SAMD)

constexpr uint16_t kTc4PeriodTicks = 469;  // Compare value for ~10 ms at 48 MHz / 1024 prescale
// How many TC4 overflows make one 2 s sample request
constexpr uint16_t kIsrTicksPerSample =  static_cast<uint16_t>(kSensorSampleMs / kTc4BasePeriodMs);

static uint16_t isrTickDivider = 0;  // Counts 10 ms ISR ticks until kIsrTicksPerSample

volatile uint8_t g_sampleTicksPending = 0;  // ISR increments; loop decrements per I2C sample

static uint8_t samplesInCurrentWindow = 0;  // Samples collected toward one 2 s average window

static float tempSum = 0.0f;  // Running sum of temperature for current window
static float humidSum = 0.0f;  // Running sum of humidity for current window
static float pressSum = 0.0f;  // Running sum of pressure for current window
static float luxSum = 0.0f;  // Running sum of lux for current window
static uint16_t bmeSampleCount = 0;  // Number of valid BME samples in current window
static uint16_t luxSampleCount = 0;  // Number of valid BH1750 samples in current window

static SensorData avgBuffer;  // Holds last completed average (for avg_datalog.csv)
static volatile bool newAverageAvailable = false;  // Set when a new average is ready to log
static volatile uint16_t ledPulseTicksRemaining = 0;  // ISR-managed LED pulse duration counter

static inline void writeActivityLed(bool on) {  // Map logical ON/OFF to physical pin level
  const uint8_t level = (on == kIsrActivityLedActiveHigh) ? HIGH : LOW;
  digitalWrite(kIsrActivityLedPin, level);
}

static inline void startActivityLedPulse() {  // Start/refresh one LED pulse from non-ISR code
  noInterrupts();
  writeActivityLed(true);
  ledPulseTicksRemaining = static_cast<uint16_t>(kIsrActivityLedPulseMs / kTc4BasePeriodMs);
  if (ledPulseTicksRemaining == 0) {
    ledPulseTicksRemaining = 1;
  }
  interrupts();
}

static void printAverageReady() {  // Visible serial notification each completed average
  Serial.print(F("[Sampler] AVG ready T="));
  Serial.print(avgBuffer.temperature, 2);
  Serial.print(F("C H="));
  Serial.print(avgBuffer.humidity, 2);
  Serial.print(F("% P="));
  Serial.print(avgBuffer.pressure, 2);
  Serial.print(F("hPa L="));
  Serial.print(avgBuffer.lux, 2);
  Serial.println(F("lx"));
}

void TC4_Handler() {  // Hardware interrupt: TC4 overflow (~10 ms base tick)
  if (TC4->COUNT16.INTFLAG.bit.OVF && TC4->COUNT16.INTENSET.bit.OVF) {  // Overflow interrupt fired
    if (ledPulseTicksRemaining > 0) {  // Keep LED ON while pulse time remains
      ledPulseTicksRemaining--;
      if (ledPulseTicksRemaining == 0) {
        writeActivityLed(false);  // End pulse: LED OFF
      }
    }
    isrTickDivider++;  // Count 10 ms ticks toward 2 s interval
    if (isrTickDivider >= kIsrTicksPerSample) {  // 200 x 10 ms = 2000 ms
      isrTickDivider = 0;  // Reset divider for next 2 s period
      g_sampleTicksPending++;  // Request one I2C sample (do not call Wire here)
    }
    TC4->COUNT16.INTFLAG.bit.OVF = 1;  // Clear overflow flag so interrupt can fire again
  }
}

static void setupTc4SampleTimer() {  // Configure GCLK and TC4 (10 ms base, divided to 2 s in ISR)
  GCLK->GENDIV.reg = GCLK_GENDIV_DIV(1) | GCLK_GENDIV_ID(4);  // GCLK4 divide-by-1
  while (GCLK->STATUS.bit.SYNCBUSY) {  // Wait for GCLK divider write
  }

  GCLK->GENCTRL.reg = GCLK_GENCTRL_IDC | GCLK_GENCTRL_GENEN |  // Enable generator 4
                      GCLK_GENCTRL_SRC_DFLL48M | GCLK_GENCTRL_ID(4);  // Source = 48 MHz DFLL
  while (GCLK->STATUS.bit.SYNCBUSY) {  // Wait for generator config
  }

  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK4 |  // Route GCLK4 to TC4/TC5
                      GCLK_CLKCTRL_ID_TC4_TC5;  // Peripheral channel TC4_TC5
  while (GCLK->STATUS.bit.SYNCBUSY) {  // Wait for clock routing
  }

  TC4->COUNT16.CC[0].reg = kTc4PeriodTicks;  // Set period (top value for match frequency)
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY) {  // Wait for compare register sync
  }

  NVIC_SetPriority(TC4_IRQn, 0);  // High priority for stable timer tick
  NVIC_EnableIRQ(TC4_IRQn);  // Enable TC4 interrupt in NVIC

  TC4->COUNT16.INTFLAG.bit.OVF = 1;  // Clear any pending overflow before enable
  TC4->COUNT16.INTENSET.bit.OVF = 1;  // Enable overflow interrupt

  TC4->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024 | TC_CTRLA_WAVEGEN_MFRQ |  // MFRQ + /1024
                              TC_CTRLA_ENABLE;  // Start TC4 counter
  while (TC4->COUNT16.STATUS.bit.SYNCBUSY) {  // Wait for enable to take effect
  }
}

static void accumulateOneSample() {  // One I2C read; add values into running sums
  readSensors();  // Update global sensorBuffer via SensorReading.cpp
  const SensorData sample = getSensorData();  // Copy latest instantaneous readings

  if (isBme280Ok()) {  // Only average BME channels when sensor is healthy
    tempSum += sample.temperature;  // Add this sample to temperature sum
    humidSum += sample.humidity;  // Add this sample to humidity sum
    pressSum += sample.pressure;  // Add this sample to pressure sum
    bmeSampleCount++;  // Count one more BME sample in this window
  }

  if (isBh1750Ok()) {  // Only average lux when light sensor is healthy
    luxSum += sample.lux;  // Add this sample to lux sum
    luxSampleCount++;  // Count one more lux sample in this window
  }
}

static void finalizeAverageWindow() {  // Compute mean over samplesInCurrentWindow and reset sums
  if (bmeSampleCount > 0) {  // Avoid divide-by-zero if BME failed all samples
    const float n = static_cast<float>(bmeSampleCount);  // Divisor = actual sample count
    avgBuffer.temperature = tempSum / n;  // Mean temperature for CSV / logging
    avgBuffer.humidity = humidSum / n;  // Mean humidity for CSV / logging
    avgBuffer.pressure = pressSum / n;  // Mean pressure for CSV / logging
  }

  if (luxSampleCount > 0) {  // Avoid divide-by-zero if light sensor failed all samples
    const float n = static_cast<float>(luxSampleCount);  // Divisor for lux mean
    avgBuffer.lux = luxSum / n;  // Mean lux for CSV / logging
  }

  avgBuffer.ready = (bmeSampleCount > 0 || luxSampleCount > 0);  // Mark average struct valid
  newAverageAvailable = avgBuffer.ready;  // Tell AvgDataLogger a new row is available
  if (avgBuffer.ready) {  // Blink activity LED when an averaged sample is produced
    startActivityLedPulse();
    printAverageReady();
  }

  tempSum = 0.0f;  // Reset sums for next 2 s window
  humidSum = 0.0f;  // Reset humidity sum
  pressSum = 0.0f;  // Reset pressure sum
  luxSum = 0.0f;  // Reset lux sum
  bmeSampleCount = 0;  // Reset BME sample counter
  luxSampleCount = 0;  // Reset lux sample counter
}

void sensorSamplerInit() {  // Call from setup() after sensorsInit()
  pinMode(kIsrActivityLedPin, OUTPUT);  // Configure ISR activity LED as output
  writeActivityLed(false);  // Start LED in OFF state
  avgBuffer.ready = false;  // No average produced yet
  newAverageAvailable = false;  // Logger should not write until first window completes
  g_sampleTicksPending = 0;  // No pending ISR ticks at start
  samplesInCurrentWindow = 0;  // Start new window at sample zero
  isrTickDivider = 0;  // Reset 10 ms → 2 s divider
  ledPulseTicksRemaining = 0;  // No pulse active at startup

  setupTc4SampleTimer();  // Start TC4 base timer on SAMD21
  Serial.print(F("[Sampler] TC4 "));  // Status message part 1
  Serial.print(kSensorSampleMs);  // Print sample interval (2000 ms)
  Serial.print(F(" ms tick, "));  // Status message part 2
  Serial.print(kSensorAverageMs);  // Print average window
  Serial.println(F(" ms average window."));  // Status message part 3 + newline
}

void sensorSamplerService() {  // Call at top of loop(); timing driven by TC4, not loop delay
  while (g_sampleTicksPending > 0) {  // Catch up on every 2 s tick the loop missed
    noInterrupts();  // Atomic decrement of volatile counter
    g_sampleTicksPending--;  // Consume one ISR tick = one sample slot
    interrupts();  // Re-enable interrupts

    accumulateOneSample();  // Perform one timed I2C sample and add to sums
    samplesInCurrentWindow++;  // Track how many samples in this average window

    if (samplesInCurrentWindow >= kSamplesPerAverageWindow) {  // 1 sample = 2 s window done
      finalizeAverageWindow();  // Publish average and set newAverageAvailable
      samplesInCurrentWindow = 0;  // Begin next window
    }
  }
}

SensorData getAveragedSensorData() {  // Used by AvgDataLogger only (not dashboard)
  return avgBuffer;  // Return copy of last completed average struct
}

bool isAveragedSensorDataReady() {  // True until AvgDataLogger calls acknowledge
  return newAverageAvailable;  // Pulse flag for one new CSV row
}

void sensorSamplerAcknowledgeAverage() {  // Called after avg_datalog.csv row is written
  newAverageAvailable = false;  // Allow detection of the next completed window
}

#else

// Non-SAMD build: software-timed sampler that mirrors SAMD behavior.
static SensorData avgBuffer;
static volatile bool newAverageAvailable = false;
static float tempSum = 0.0f;
static float humidSum = 0.0f;
static float pressSum = 0.0f;
static float luxSum = 0.0f;
static uint16_t bmeSampleCount = 0;
static uint16_t luxSampleCount = 0;
static uint8_t samplesInCurrentWindow = 0;
static unsigned long lastSampleMs = 0;
static unsigned long ledPulseOffAtMs = 0;

static inline void writeActivityLed(bool on) {
  const uint8_t level = (on == kIsrActivityLedActiveHigh) ? HIGH : LOW;
  digitalWrite(kIsrActivityLedPin, level);
}

static void startActivityLedPulse() {
  writeActivityLed(true);
  ledPulseOffAtMs = millis() + kIsrActivityLedPulseMs;
}

static void printAverageReady() {
  Serial.print(F("[Sampler] AVG ready T="));
  Serial.print(avgBuffer.temperature, 2);
  Serial.print(F("C H="));
  Serial.print(avgBuffer.humidity, 2);
  Serial.print(F("% P="));
  Serial.print(avgBuffer.pressure, 2);
  Serial.print(F("hPa L="));
  Serial.print(avgBuffer.lux, 2);
  Serial.println(F("lx"));
}

static void accumulateOneSample() {
  readSensors();
  const SensorData sample = getSensorData();
  if (isBme280Ok()) {
    tempSum += sample.temperature;
    humidSum += sample.humidity;
    pressSum += sample.pressure;
    bmeSampleCount++;
  }
  if (isBh1750Ok()) {
    luxSum += sample.lux;
    luxSampleCount++;
  }
}

static void finalizeAverageWindow() {
  if (bmeSampleCount > 0) {
    const float n = static_cast<float>(bmeSampleCount);
    avgBuffer.temperature = tempSum / n;
    avgBuffer.humidity = humidSum / n;
    avgBuffer.pressure = pressSum / n;
  }
  if (luxSampleCount > 0) {
    avgBuffer.lux = luxSum / static_cast<float>(luxSampleCount);
  }

  avgBuffer.ready = (bmeSampleCount > 0 || luxSampleCount > 0);
  newAverageAvailable = avgBuffer.ready;
  if (avgBuffer.ready) {
    startActivityLedPulse();
    printAverageReady();
  }

  tempSum = 0.0f;
  humidSum = 0.0f;
  pressSum = 0.0f;
  luxSum = 0.0f;
  bmeSampleCount = 0;
  luxSampleCount = 0;
}

void sensorSamplerInit() {
  pinMode(kIsrActivityLedPin, OUTPUT);
  writeActivityLed(false);
  avgBuffer.ready = false;
  newAverageAvailable = false;
  samplesInCurrentWindow = 0;
  tempSum = humidSum = pressSum = luxSum = 0.0f;
  bmeSampleCount = luxSampleCount = 0;
  lastSampleMs = millis();
  ledPulseOffAtMs = 0;
  Serial.println(F("[Sampler] SW timing active (non-SAMD core)."));
}

void sensorSamplerService() {
  const unsigned long now = millis();

  if (ledPulseOffAtMs != 0 && static_cast<long>(now - ledPulseOffAtMs) >= 0) {
    writeActivityLed(false);
    ledPulseOffAtMs = 0;
  }

  if (static_cast<unsigned long>(now - lastSampleMs) >= kSensorSampleMs) {
    lastSampleMs += kSensorSampleMs;
    accumulateOneSample();
    samplesInCurrentWindow++;

    if (samplesInCurrentWindow >= kSamplesPerAverageWindow) {
      finalizeAverageWindow();
      samplesInCurrentWindow = 0;
    }
  }
}

SensorData getAveragedSensorData() {
  return avgBuffer;
}

bool isAveragedSensorDataReady() {
  return newAverageAvailable;
}

void sensorSamplerAcknowledgeAverage() {
  newAverageAvailable = false;
}

#endif
