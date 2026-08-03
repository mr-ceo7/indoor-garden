/*
 * ============================================================
 *  SMART INDOOR GARDEN  —  Arduino Uno Slave Driver (DAQ & Actuators)
 * ============================================================
 *  Acts as a dedicated peripheral for the ESP-01. Handles:
 *    - DHT11 Temperature & Humidity Sensor (Pin 5)
 *    - pH Sensor (Simulated mathematically due to lack of hardware sensor)
 *    - Nutrient Sensor / EC (Analog A1)
 *    - Submersible Pump (Pin 9)
 *    - Piezo Buzzer (Pin 8)
 *    - I2C 16x2 LCD Display (0x27 I2C address)
 *    - Responds to UART serial commands from the ESP-01 master.
 *
 *  YSK 2026  |   Kisii School  |  Predictive Hydroponic Optimization
 * ============================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Bonezegei_DHT11.h>

// ─── Pin Definitions ────────────────────────────────────────
#define BUZZER_PIN 11
#define PUMP_PIN 9
#define DHT_PIN 5
#define PH_PIN A0          // Keep defined, but pH is simulated
#define NUTRIENT_PIN A1

// ─── LCD Setup ──────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ─── Sensor Setup ───────────────────────────────────────────
Bonezegei_DHT11 dht(DHT_PIN);

// ─── System Modes ───────────────────────────────────────────
enum Mode { MODE_REACTIVE, MODE_AI, MODE_MANUAL };
Mode currentMode = MODE_REACTIVE;

// ─── Timing Variables (Non-blocking execution) ──────────────
unsigned long lastTelemetryTime = 0;
unsigned long lastLcdUpdateTime = 0;
unsigned long lastReactiveCheckTime = 0;
unsigned long dosingEndTime = 0;
bool isDosing = false;

const unsigned long TELEMETRY_INTERVAL = 500;   // Telemetry every 500ms
const unsigned long LCD_UPDATE_INTERVAL = 2000; // Refresh screen every 2s
const unsigned long REACTIVE_CHECK_INTERVAL = 1000; // Check NC every 1s

// ─── Dosing State (Reactive Mode) ───────────────────────────
bool reactiveDosingActive = false;
unsigned long reactiveDosingEndTime = 0;
unsigned long reactiveCooldownEndTime = 0;

// ─── Simulated pH Variables ──────────────────────────────────
float simulatedPh = 6.5;
unsigned long lastPhSimTime = 0;

// ─── Serial Communication Buffer ─────────────────────────────
char rxBuf[48];
byte rxLen = 0;

// ─── Buzzer Melodies ────────────────────────────────────────
void playTone(unsigned int frequency, unsigned long duration_ms) {
  if (frequency == 0) {
    delay(duration_ms);
    return;
  }
  tone(BUZZER_PIN, frequency, duration_ms);
  delay(duration_ms);
  noTone(BUZZER_PIN);
}

void beepClick() { playTone(4000, 20); }

void beepWelcome() {
  playTone(523, 100);
  delay(20);
  playTone(659, 100);
  delay(20);
  playTone(784, 150);
}

void beepDone() {
  playTone(2000, 80);
  delay(40);
  playTone(2000, 80);
}

void beepError() {
  for (int i = 0; i < 3; i++) {
    playTone(200, 80);
    delay(20);
  }
}

void beepSuccess() {
  playTone(1000, 50);
  delay(30);
  playTone(1500, 50);
  delay(30);
  playTone(2000, 50);
  delay(30);
  playTone(2500, 100);
}

// Mode Transitions
void beepModeReactive() {
  playTone(800, 60);
  delay(30);
  playTone(1000, 80);
}

void beepModeAI() {
  playTone(1000, 50);
  delay(20);
  playTone(1300, 50);
  delay(20);
  playTone(1600, 80);
}

void beepModeManual() {
  playTone(600, 70);
  delay(30);
  playTone(800, 70);
}

// ─── Serial Command Handler ──────────────────────────────────
void handleCommand(char *cmd) {
  if (strncmp(cmd, "MODE:REACTIVE", 13) == 0) {
    currentMode = MODE_REACTIVE;
    reactiveDosingActive = false;
    isDosing = false;
    digitalWrite(PUMP_PIN, HIGH); // Ensure pump is OFF
    beepModeReactive();
  } else if (strncmp(cmd, "MODE:AI", 7) == 0) {
    currentMode = MODE_AI;
    reactiveDosingActive = false;
    isDosing = false;
    digitalWrite(PUMP_PIN, HIGH); // Ensure pump is OFF
    beepModeAI();
  } else if (strncmp(cmd, "MODE:MANUAL", 11) == 0) {
    currentMode = MODE_MANUAL;
    reactiveDosingActive = false;
    isDosing = false;
    digitalWrite(PUMP_PIN, HIGH); // Ensure pump is OFF
    beepModeManual();
  } else if (strncmp(cmd, "PUMP:ON", 7) == 0) {
    if (currentMode == MODE_MANUAL || currentMode == MODE_AI) {
      digitalWrite(PUMP_PIN, LOW); // ON
      isDosing = false; // cancel timed dosing
      beepClick();
    }
  } else if (strncmp(cmd, "PUMP:OFF", 8) == 0) {
    if (currentMode == MODE_MANUAL || currentMode == MODE_AI) {
      digitalWrite(PUMP_PIN, HIGH); // OFF
      isDosing = false;
      beepClick();
    }
  } else if (strncmp(cmd, "DOSING:", 7) == 0) {
    float duration = atof(cmd + 7);
    if (duration > 0.0 && (currentMode == MODE_MANUAL || currentMode == MODE_AI)) {
      digitalWrite(PUMP_PIN, LOW); // ON
      dosingEndTime = millis() + (unsigned long)(duration * 1000.0);
      isDosing = true;
      beepClick();
    }
  } else if (strncmp(cmd, "BEEP:CLICK", 10) == 0) {
    beepClick();
  } else if (strncmp(cmd, "BEEP:WELCOME", 12) == 0) {
    beepWelcome();
  } else if (strncmp(cmd, "BEEP:DONE", 9) == 0) {
    beepDone();
  } else if (strncmp(cmd, "BEEP:ERROR", 10) == 0) {
    beepError();
  } else if (strncmp(cmd, "BEEP:SUCCESS", 12) == 0) {
    beepSuccess();
  }
}

// ─── Setup ──────────────────────────────────────────────────
void setup() {
  digitalWrite(PUMP_PIN, HIGH); // Start with pump OFF (active-low relay)
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  Serial.begin(9600);
  dht.begin();
  Wire.begin();
  
  lcd.init();
  lcd.backlight();
  
  lcd.print("Smart indoor");
  lcd.setCursor(0, 1);
  lcd.print("    garden!");
  
  beepWelcome();
  delay(2000);
  lcd.clear();
}

// ─── Main Loop ──────────────────────────────────────────────
void loop() {
  unsigned long currentMillis = millis();

  // 1. Read Physical Sensors (DHT11 & Nutrient/EC)
  float temp = 0.0;
  int hum = 0;
  if (dht.getData()) {
    temp = dht.getTemperature();
    hum = dht.getHumidity();
  }
  int nc = analogRead(NUTRIENT_PIN);

  // 2. Mathematically Simulate pH Sensor Behavior (Dynamic Digital Twin)
  // Run simulation step every 2 seconds
  if (currentMillis - lastPhSimTime >= 2000) {
    lastPhSimTime = currentMillis;
    
    int pumpActive = (digitalRead(PUMP_PIN) == LOW) ? 1 : 0;
    if (pumpActive) {
      // Pump running simulates dosing, which neutralizes/reduces pH back to optimal (e.g. 6.0)
      if (simulatedPh > 6.0) {
        simulatedPh -= 0.15;
        if (simulatedPh < 6.0) simulatedPh = 6.0;
      } else if (simulatedPh < 6.0) {
        simulatedPh += 0.15;
        if (simulatedPh > 6.0) simulatedPh = 6.0;
      }
    } else {
      // Slowly drift pH upward over time (typical in water gardens as plants feed)
      // Drift up by 0.02 units every 2 seconds
      simulatedPh += 0.02;
      if (simulatedPh > 8.2) simulatedPh = 8.2; // Cap drift
    }
    
    // Add micro-fluctuations (measurement noise) of +/- 0.02 pH units
    simulatedPh += (random(-2, 3) / 100.0);
    simulatedPh = constrain(simulatedPh, 0.0, 14.0);
  }

  // 3. Handle Timed Dosing (Manual/AI modes)
  if (isDosing) {
    if (currentMillis >= dosingEndTime) {
      digitalWrite(PUMP_PIN, HIGH); // Turn OFF pump
      isDosing = false;
      beepDone();
    }
  }

  // 4. Handle Serial Communication (Non-blocking)
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (rxLen > 0) {
        rxBuf[rxLen] = '\0';
        handleCommand(rxBuf);
        rxLen = 0;
      }
    } else if (rxLen < sizeof(rxBuf) - 1) {
      rxBuf[rxLen++] = c;
    }
  }

  // 5. Reactive Mode Logic (Non-blocking)
  if (currentMode == MODE_REACTIVE) {
    if (currentMillis - lastReactiveCheckTime >= REACTIVE_CHECK_INTERVAL) {
      lastReactiveCheckTime = currentMillis;

      if (!reactiveDosingActive && (currentMillis >= reactiveCooldownEndTime)) {
        if (nc >= 507 || nc < 50) {
          // Conditions bad - trigger nutrient pump
          digitalWrite(PUMP_PIN, LOW); // Turn ON
          reactiveDosingActive = true;
          reactiveDosingEndTime = currentMillis + 3600; // 3.6s run time
          
          // Sound alarm
          playTone(1000, 200);
        }
      }
    }

    // Handle reactive dosing timer
    if (reactiveDosingActive && (currentMillis >= reactiveDosingEndTime)) {
      digitalWrite(PUMP_PIN, HIGH); // Turn OFF pump
      reactiveDosingActive = false;
      reactiveCooldownEndTime = currentMillis + 10000; // 10s cooldown
      beepDone();
    }
  }

  // 6. Send Telemetry Packet to ESP-01 Master
  if (currentMillis - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    lastTelemetryTime = currentMillis;
    
    // Determine active pump state
    int pumpState = (digitalRead(PUMP_PIN) == LOW) ? 1 : 0;
    
    // Packet structure: DATA:temp,hum,ph,nc,pumpState
    Serial.print(F("DATA:"));
    Serial.print(temp, 1);
    Serial.print(F(","));
    Serial.print(hum);
    Serial.print(F(","));
    Serial.print(simulatedPh, 2); // Send the simulated pH value
    Serial.print(F(","));
    Serial.print(nc);
    Serial.print(F(","));
    Serial.println(pumpState);
  }

  // 7. Update local 16x2 LCD display (Cycled screen modes)
  if (currentMillis - lastLcdUpdateTime >= LCD_UPDATE_INTERVAL) {
    lastLcdUpdateTime = currentMillis;
    static byte lcdScreen = 0;
    lcd.clear();

    int pumpActive = (digitalRead(PUMP_PIN) == LOW) ? 1 : 0;

    if (pumpActive) {
      lcd.setCursor(0, 0);
      lcd.print(F("PUMP ACTIVE!!   "));
      lcd.setCursor(0, 1);
      if (currentMode == MODE_REACTIVE) {
        lcd.print(F("Dosing: NPK LOW "));
      } else {
        lcd.print(F("Dosing nutrients"));
      }
    } else {
      if (lcdScreen == 0) {
        // Show sensor parameters
        lcd.setCursor(0, 0);
        lcd.print(F("T:"));
        lcd.print(temp, 1);
        lcd.print(F("C H:"));
        lcd.print(hum);
        lcd.print(F("%   "));
        
        lcd.setCursor(0, 1);
        lcd.print(F("pH:"));
        lcd.print(simulatedPh, 1);
        lcd.print(F(" EC:"));
        lcd.print(nc);
        lcd.print(F("    "));
        
        lcdScreen = 1;
      } else {
        // Show mode and conditions status
        lcd.setCursor(0, 0);
        lcd.print(F("MODE: "));
        if (currentMode == MODE_REACTIVE) lcd.print(F("REACTIVE"));
        else if (currentMode == MODE_AI) lcd.print(F("AI-EDGE "));
        else if (currentMode == MODE_MANUAL) lcd.print(F("MANUAL  "));

        lcd.setCursor(0, 1);
        lcd.print(F("COND: "));
        if (nc >= 507 || nc < 50) {
          lcd.print(F("BAD (ALERT) "));
        } else {
          lcd.print(F("GOOD        "));
        }
        
        lcdScreen = 0;
      }
    }
  }
}
