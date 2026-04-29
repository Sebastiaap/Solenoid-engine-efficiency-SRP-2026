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

#define SOLENOID_ON_MS 100
#define SOLENOID_INTERVAL_MS 1000
#define SOLENOID_STEP_MS 200
#define LOG_INTERVAL_MS 200

Adafruit_INA219 ina219;

unsigned long lastSequenceTime = 0;
unsigned long lastLogTime = 0;
unsigned long solenoidOnTime = 0;

int  currentSolenoid = -1;
bool firingActive = false;

volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseDuration = 0;
volatile bool newPulse = false;

void IRAM_ATTR onHallPulse() {
  unsigned long now = millis();
  pulseDuration = now - lastPulseTime;
  lastPulseTime = now;
  newPulse = true;
}

float calculateRPM() {
  if (pulseDuration == 0) return 0;
  return 60000.0 / pulseDuration;
}

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
      if (millis() - solenoidOnTime >= SOLENOID_STEP_MS) {
        digitalWrite(solenoids[currentSolenoid], HIGH);
        solenoidOnTime = millis();
      }
    } else {
      firingActive = false;
      currentSolenoid = -1;
    }
  }
}

void logSensors() {
  float voltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA() / 1000.0;
  float power = ina219.getPower_mW();
  float rpm;
  if (millis() - lastPulseTime > 2000) {
    rpm = 0;
  } else {
    rpm = calculateRPM();
  }

  Serial.print(millis());   
  Serial.print(",");
  Serial.print(voltage, 2); 
  Serial.print(",");
  Serial.print(current, 3);
  Serial.print(",");
  Serial.print(power, 2);   
  Serial.print(",");
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
