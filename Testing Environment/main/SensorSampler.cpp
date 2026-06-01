#include "SensorSampler.h"
#include "HardwareConfig.h"

#include <DueTimer.h>

// DueTimer period in microseconds (2000 ms = 2 s sample tick)
constexpr long SensorSamplePeriodInMicroSec = SensorSamplerInMs * 1000UL;

static volatile bool SampleAverageState = false;

static uint8_t samplesInCurrentWindow = 0;

static float tempSum = 0.0f;
static float humidSum = 0.0f;
static float pressSum = 0.0f;
static float luxSum = 0.0f;
static uint16_t bmeSampleCount = 0;
static uint16_t luxSampleCount = 0;

static SensorData avgBuffer;
static volatile bool newAverageAvailable = false;
static volatile unsigned long ledPulseOffAtMs = 0;

// Start the interupt timer, called in Setup() in main.ino
void sensorSamplerInit() {
  pinMode(LedPin, OUTPUT);
  setActivityLed(false);

  SampleAverageState = false;
  avgBuffer.ready = false;
  newAverageAvailable = false;
  samplesInCurrentWindow = 0;
  ledPulseOffAtMs = 0;
  tempSum = humidSum = pressSum = luxSum = 0.0f;
  bmeSampleCount = luxSampleCount = 0;

  Timer6.attachInterrupt(onSampleTimerISR);
  Timer6.setPeriod(SensorSamplePeriodInMicroSec);
  Timer6.start();

  Serial.print(F("[SensorSampler] DueTimer6 every "));
  Serial.print(SensorSamplerInMs);
  Serial.println(F(" ms (hardware timer)."));
}

// ISR to request a sample every 2s
static void onSampleTimerISR() {
  SampleAverageState = true;
}

// LED to pulse for 200ms after a successful average
static void serviceActivityLed() {
  if (ledPulseOffAtMs == 0) {
    return;
  }
  if (static_cast<long>(millis() - ledPulseOffAtMs) >= 0) {
    setActivityLed(false);
    ledPulseOffAtMs = 0;
  }
}

// Called from loop(), Read sensors and build averages
void sensorSamplerService() {
  serviceActivityLed();

  if (!SampleAverageState) {
    return;
  }
  // Time-Freezing the Timer to intefere
  noInterrupts();
  SampleAverageState = false;
  interrupts();

  accumulateOneSample();
  samplesInCurrentWindow++;

  if (samplesInCurrentWindow >= NumberOfSamplePerAvg) {
    finalizeAverageWindow();
    samplesInCurrentWindow = 0;
  }
}

// Sum up all the data to make 1 sample
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

// Calculate average result after n=10 samples and reset sum&count to 0 for next trial
static void finalizeAverageWindow() {
  if (bmeSampleCount > 0) {
    const float n = bmeSampleCount;
    avgBuffer.temperature = tempSum / n;
    avgBuffer.humidity = humidSum / n;
    avgBuffer.pressure = pressSum / n;
  }

  if (luxSampleCount > 0) {
    avgBuffer.lux = luxSum / luxSampleCount;
  }

  avgBuffer.ready = (bmeSampleCount > 0 || luxSampleCount > 0);
  newAverageAvailable = avgBuffer.ready;

  if (avgBuffer.ready) {
    pulseActivityLed();  
    printAverageReady();
  } else {
    Serial.println(F("[Sampler] AVG skipped (no valid sensor samples)."));
  }

  tempSum = 0.0f;
  humidSum = 0.0f;
  pressSum = 0.0f;
  luxSum = 0.0f;
  bmeSampleCount = 0;
  luxSampleCount = 0;
}

// D9 LED stays on for 200ms after a successful average
static void pulseActivityLed() {
  setActivityLed(true);
  ledPulseOffAtMs = millis() + LedPulseMs;
}

//  Turn the D9 LED on or off
static void setActivityLed(bool state) {
  digitalWrite(LedPin, (state) ? HIGH : LOW);
}

// Print successful Sampling Average
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

// Average Data saved in avgBuffer class, ready for access
SensorData getAveragedSensorData() {
  return avgBuffer;
}

// Average Sensor Data is Ready or Not, , ready for access
bool isAveragedSensorDataReady() {
  return newAverageAvailable;
}

// Clear flag so same average is not logged twice, , ready for access
void sensorSamplerAcknowledgeAverage() {
  newAverageAvailable = false;
}
