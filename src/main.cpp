#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

#define HALL_PIN D12

#define SOLENOID0_PIN D2
#define SOLENOID1_PIN D3
#define SOLENOID2_PIN D4
#define SOLENOID3_PIN D5
#define SOLENOID4_PIN D6
#define SOLENOID5_PIN D7

uint8_t solenoids[] = {SOLENOID0_PIN, SOLENOID4_PIN, SOLENOID2_PIN, SOLENOID5_PIN, SOLENOID1_PIN, SOLENOID3_PIN};
const int SOLENOID_COUNT = sizeof(solenoids) / sizeof(solenoids[0]);

#define SOLENOID_ON_MS         100
#define SOLENOID_STEP_START_MS 300
#define SOLENOID_STEP_MIN_MS    50
#define ACCEL_DURATION_MS      15000
#define LOG_INTERVAL_MS         200

Adafruit_INA219 ina219;

unsigned long lastLogTime    = 0;
unsigned long solenoidOnTime = 0;
unsigned long startTime      = 0;

float currentStepMs = SOLENOID_STEP_START_MS;

int currentSolenoid = 0;

volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseDuration = 0;

void IRAM_ATTR onHallPulse() {
  unsigned long now = millis();
  pulseDuration = now - lastPulseTime;
  lastPulseTime = now;
}

float calculateRPM() {
  if (pulseDuration == 0) return 0;
  return 60000.0 / pulseDuration;
}

void updateTiming() {
  float progress = (float)(millis() - startTime) / ACCEL_DURATION_MS;
  if (progress > 1.0) progress = 1.0;
  currentStepMs = SOLENOID_STEP_START_MS - (SOLENOID_STEP_START_MS - SOLENOID_STEP_MIN_MS) * progress;
}

void updateFiringSequence() {
  if (millis() - solenoidOnTime >= SOLENOID_ON_MS) {
    digitalWrite(solenoids[currentSolenoid], LOW);
    currentSolenoid = (currentSolenoid + 1) % SOLENOID_COUNT;

    if (millis() - solenoidOnTime >= (unsigned long)currentStepMs) {
      digitalWrite(solenoids[currentSolenoid], HIGH);
      solenoidOnTime = millis();
    }
  }
}

void logSensors() {
  float voltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA() / 1000.0;
  float power   = ina219.getPower_mW();
  float rpm     = (millis() - lastPulseTime > 2000) ? 0 : calculateRPM();

  Serial.print(millis());   Serial.print(",");
  Serial.print(voltage, 2); Serial.print(",");
  Serial.print(current, 3); Serial.print(",");
  Serial.print(power, 2);   Serial.print(",");
  Serial.println(rpm, 1);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("timestamp_ms,voltage_V,current_A,power_mW,rpm");

  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), onHallPulse, FALLING);

  if (!ina219.begin()) {
    Serial.println("ERROR - INA219 not found");
    while (1) { delay(10); }
  }

  for (int i = 0; i < SOLENOID_COUNT; i++) {
    pinMode(solenoids[i], OUTPUT);
    digitalWrite(solenoids[i], LOW);
  }

  startTime      = millis();
  solenoidOnTime = millis();
  digitalWrite(solenoids[0], HIGH);
}

void loop() {
  updateTiming();
  updateFiringSequence();

  if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = millis();
    logSensors();
  }
}
