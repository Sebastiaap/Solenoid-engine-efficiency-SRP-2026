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

#define SOLENOID_ON_MS 100 // Duration solenoid stays on per firing
#define SOLENOID_INTERVAL_MS 1000 // How often all solenoids fire
#define LOG_INTERVAL_MS 200 // How often sensors are logged

Adafruit_INA219 ina219;

unsigned long lastSolenoidTime = 0;
unsigned long lastLogTime      = 0;

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

  // Initialize all solenoid pins
  for (int i = 0; i < SOLENOID_COUNT; i++) {
    pinMode(solenoids[i], OUTPUT);
    digitalWrite(solenoids[i], LOW);
  }
  Serial.println("[SOLENOIDS] Ready");

  Serial.println("===============================");
}

void loop() {
  unsigned long now = millis();

  // Fire all solenoids sequentially every SOLENOID_INTERVAL_MS
  if (now - lastSolenoidTime >= SOLENOID_INTERVAL_MS) {
    lastSolenoidTime = now;

    for (int i = 0; i < SOLENOID_COUNT; i++) {
      digitalWrite(solenoids[i], HIGH);
      delay(SOLENOID_ON_MS);
      digitalWrite(solenoids[i], LOW);
      Serial.println("[SOLENOID] Fired solenoid " + String(i));
    }
  }

  // Log sensors every LOG_INTERVAL_MS
  if (now - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = now;

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
}
