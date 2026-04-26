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

uint8_t solenoids[] = {SOLENOID0_PIN, SOLENOID1_PIN, SOLENOID2_PIN, SOLENOID3_PIN, SOLENOID4_PIN, SOLENOID5_PIN};
const int SOLENOID_COUNT = sizeof(solenoids) / sizeof(solenoids[0]);

#define SOLENOID_ON_MS 100
#define SOLENOID_INTERVAL_MS 1000
#define LOG_INTERVAL_MS 200

Adafruit_INA219 ina219;

unsigned long lastSequenceTime = 0;
unsigned long lastLogTime = 0;
unsigned long solenoidOnTime = 0;

int  currentSolenoid = -1;
bool firingActive = false;

void startFiringSequence() {
  firingActive = true;
  currentSolenoid = 0;
  digitalWrite(solenoids[0], HIGH);
  solenoidOnTime = millis();
}

void updateFiringSequence() {
  if (!firingActive) return;

  if (millis() - solenoidOnTime >= SOLENOID_ON_MS) {
    digitalWrite(solenoids[currentSolenoid], LOW);
    currentSolenoid++;

    if (currentSolenoid < SOLENOID_COUNT) {
      digitalWrite(solenoids[currentSolenoid], HIGH);
      solenoidOnTime = millis();
    } else {
      firingActive = false;
      currentSolenoid = -1;
    }
  }
}

void logSensors() {
  Serial.println("[INA219] Voltage: " + String(ina219.getBusVoltage_V(), 2) + " V");
  Serial.println("[INA219] Current: " + String(ina219.getCurrent_mA() / 1000.0, 3) + " A");
  Serial.println("[INA219] Power: "   + String(ina219.getPower_mW(), 2) + " mW");

  if (digitalRead(HALL_PIN) == HIGH) {
    Serial.println("[HALL] Magnetic field detected");
  } else {
    Serial.println("[HALL] No magnetic field detected.");
  }

  Serial.println("---");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("===============================");

  pinMode(HALL_PIN, INPUT_PULLUP);
  Serial.println("[HALL] Ready");

  if (!ina219.begin()) {
    Serial.println("[INA219] ERROR - Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  Serial.println("[INA219] Ready");

  for (int i = 0; i < SOLENOID_COUNT; i++) {
    pinMode(solenoids[i], OUTPUT);
    digitalWrite(solenoids[i], LOW);
  }
  Serial.println("[SOLENOIDS] Ready");

  Serial.println("===============================");
}

void loop() {
  unsigned long now = millis();

  if (!firingActive && now - lastSequenceTime >= SOLENOID_INTERVAL_MS) {
    lastSequenceTime = now;
    startFiringSequence();
  }

  updateFiringSequence();

  if (now - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = now;
    logSensors();
  }
}
