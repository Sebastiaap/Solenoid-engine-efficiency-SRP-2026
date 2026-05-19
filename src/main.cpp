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

uint8_t solenoids[] = {SOLENOID0_PIN, SOLENOID4_PIN, SOLENOID2_PIN,
                       SOLENOID5_PIN, SOLENOID1_PIN, SOLENOID3_PIN};
const int SOLENOID_COUNT = sizeof(solenoids) / sizeof(solenoids[0]);

// Each solenoid fires for currentOnMs, then the slot lasts SOLENOID_SLOT_MS total.
// Slot must always be >= ON duration.
#define SOLENOID_SLOT_MS      800   // Fixed time slice per solenoid
#define SOLENOID_ON_START_MS  780   // ON duration at t=0 (nearly full slot)
#define SOLENOID_ON_MIN_MS    100   // ON duration at end of accel
#define ACCEL_DURATION_MS   60000   // 60 seconds to ramp up
#define LOG_INTERVAL_MS       200

Adafruit_INA219 ina219;

unsigned long lastLogTime    = 0;
unsigned long slotStartTime  = 0;   // When the current solenoid's slot began
unsigned long startTime      = 0;

float currentOnMs = SOLENOID_ON_START_MS;  // Shrinks over time

int currentSolenoid = 0;
bool solenoidActive = false;

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
  currentOnMs = SOLENOID_ON_START_MS
                - (SOLENOID_ON_START_MS - SOLENOID_ON_MIN_MS) * progress;
}

void updateFiringSequence() {
  unsigned long now     = millis();
  unsigned long elapsed = now - slotStartTime;

  // 1) Turn OFF once the ON window has passed
  if (solenoidActive && elapsed >= (unsigned long)currentOnMs) {
    digitalWrite(solenoids[currentSolenoid], LOW);
    solenoidActive = false;
  }

  // 2) Advance to next solenoid once the full slot has passed
  if (elapsed >= SOLENOID_SLOT_MS) {
    // Safety: make sure current is off before moving on
    if (solenoidActive) {
      digitalWrite(solenoids[currentSolenoid], LOW);
      solenoidActive = false;
    }

    currentSolenoid = (currentSolenoid + 1) % SOLENOID_COUNT;
    slotStartTime   = now;

    digitalWrite(solenoids[currentSolenoid], HIGH);
    solenoidActive = true;
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

  startTime     = millis();
  slotStartTime = millis();

  // Fire the first solenoid immediately
  digitalWrite(solenoids[0], HIGH);
  solenoidActive = true;
}

void loop() {
  updateTiming();
  updateFiringSequence();

  if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = millis();
    logSensors();
  }
}