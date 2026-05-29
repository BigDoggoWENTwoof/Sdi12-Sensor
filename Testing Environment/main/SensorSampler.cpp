#include "SensorSampler.h"
#include "HardwareConfig.h"

// This project targets Arduino Due (ATSAM3X8E) only.
#ifndef __SAM3X8E__
#error "Wrong board selected. In Arduino IDE choose: Tools > Board > Arduino Due."
#endif

#include <sam.h>

// How many 10 ms hardware ticks make one kSensorSampleMs (2000 ms) sample request
constexpr uint16_t kIsrTicksPerSample =
    static_cast<uint16_t>(kSensorSampleMs / kHwTimerBasePeriodMs);

// Due: TIMER_CLOCK4 = MCK/128; 10 ms tick => 100 Hz
constexpr uint32_t kHwTimerTickHz = 1000U / kHwTimerBasePeriodMs;

static uint16_t isrTickDivider = 0;
volatile uint8_t g_sampleTicksPending = 0;
volatile uint32_t g_hwTimerIsrCount = 0;  // Debug: increments every ~10 ms if ISR runs
static uint8_t samplesInCurrentWindow = 0;

static float tempSum = 0.0f;
static float humidSum = 0.0f;
static float pressSum = 0.0f;
static float luxSum = 0.0f;
static uint16_t bmeSampleCount = 0;
static uint16_t luxSampleCount = 0;

static SensorData avgBuffer;
static volatile bool newAverageAvailable = false;
static volatile unsigned long ledPulseOffAtMs = 0;  // 0 = LED pulse inactive

static inline void writeActivityLed(bool on) {
  const uint8_t level = (on == kIsrActivityLedActiveHigh) ? HIGH : LOW;
  digitalWrite(kIsrActivityLedPin, level);
}

// Turn LED on for exactly kIsrActivityLedPulseMs after a successful average (main loop timing).
static void startActivityLedPulse() {
  writeActivityLed(true);
  ledPulseOffAtMs = millis() + kIsrActivityLedPulseMs;
}

static void serviceActivityLedPulse() {
  if (ledPulseOffAtMs == 0) {
    return;
  }
  if (static_cast<long>(millis() - ledPulseOffAtMs) >= 0) {
    writeActivityLed(false);
    ledPulseOffAtMs = 0;
  }
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

// Called from TC1_Handler every ~10 ms — no I2C/Wire/Serial here
static void onHwTimerTick() {
  g_hwTimerIsrCount++;
  isrTickDivider++;
  if (isrTickDivider >= kIsrTicksPerSample) {
    isrTickDivider = 0;
    g_sampleTicksPending++;
  }
}

// Due timer channel TC1 = TC0 block, channel 1 (not used by default Servo library).
// MUST call TC_GetStatus() every entry or the interrupt will not re-arm (SAM3X requirement).
void TC1_Handler() {
  TC_GetStatus(TC0, 1);
  onHwTimerTick();
}

static void setupHwSampleTimer() {
  // Pattern from Arduino Due timer examples (Copperhill / forum.arduino.cc #130423)
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC1);

  TC_Configure(TC0, 1,
               TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4);

  const uint32_t rc = VARIANT_MCK / 128U / kHwTimerTickHz;
  TC_SetRA(TC0, 1, rc / 2U);
  TC_SetRC(TC0, 1, rc);
  TC_Start(TC0, 1);

  TC0->TC_CHANNEL[1].TC_IER = TC_IER_CPCS;
  TC0->TC_CHANNEL[1].TC_IDR = ~TC_IER_CPCS;

  NVIC_ClearPendingIRQ(TC1_IRQn);
  NVIC_SetPriority(TC1_IRQn, 0);
  NVIC_EnableIRQ(TC1_IRQn);
}

static void printBoardIdentity() {
  const uint32_t rc = VARIANT_MCK / 128U / kHwTimerTickHz;
  Serial.print(F("[Sampler] MCK="));
  Serial.print(VARIANT_MCK);
  Serial.print(F(" Hz, TC0 ch1 RC="));
  Serial.print(rc);
  Serial.print(F(" ("));
  Serial.print(kHwTimerTickHz);
  Serial.println(F(" Hz tick)"));
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
    const float n = static_cast<float>(luxSampleCount);
    avgBuffer.lux = luxSum / n;
  }

  avgBuffer.ready = (bmeSampleCount > 0 || luxSampleCount > 0);
  newAverageAvailable = avgBuffer.ready;
  if (avgBuffer.ready) {
    startActivityLedPulse();  // 200 ms ON after successful average
    printAverageReady();
  } else {
    Serial.println(F("[Sampler] AVG skipped (no valid sensor samples this window)."));
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
  g_sampleTicksPending = 0;
  samplesInCurrentWindow = 0;
  isrTickDivider = 0;
  ledPulseOffAtMs = 0;

  setupHwSampleTimer();
  printBoardIdentity();

  Serial.print(F("[Sampler] HW timer "));
  Serial.print(kSensorSampleMs);
  Serial.print(F(" ms sample, "));
  Serial.print(kSensorAverageMs);
  Serial.println(F(" ms average."));
}

void sensorSamplerService() {
  serviceActivityLedPulse();  // End 200 ms LED pulse when elapsed

  static bool timerAliveReported = false;
  if (!timerAliveReported && g_hwTimerIsrCount > 0) {
    timerAliveReported = true;
    Serial.println(F("[Sampler] HW timer ISR is running."));
  }

  while (g_sampleTicksPending > 0) {
    noInterrupts();
    g_sampleTicksPending--;
    interrupts();

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
