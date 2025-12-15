#include <Arduino.h>

#define BUZZER_PIN 18  // Try GPIO 18 instead - usually higher power

void init_buzzer() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void run_buzzer() {
    Serial.println("🔊 Buzzer ON - LOUD MODE");
    
    // Use PWM tone for much louder sound
    for (int i = 0; i < 5; i++) {
        // Generate 2000Hz tone using PWM
        for (int j = 0; j < 400; j++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(250);  // 2000Hz frequency
            digitalWrite(BUZZER_PIN, LOW);
            delayMicroseconds(250);
        }
        delay(100);  // Short pause between beeps
    }
    
    Serial.println("🔇 Buzzer OFF");
}