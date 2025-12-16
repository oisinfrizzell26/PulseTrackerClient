#include <Arduino.h>

extern void init_buzzer();
extern void run_buzzer();

void setup() {
    Serial.begin(115200);
    Serial.println("🚀 ESP32 Buzzer Test Starting");
    init_buzzer();
}

void loop() {
    Serial.println("⏰ 10 second timer - triggering buzzer");
    run_buzzer();
    Serial.println("💤 Waiting 10 seconds...");
    delay(10000);
}