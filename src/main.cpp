#include <Arduino.h>

// from mqtt_client.cpp
extern void mqtt_init();
extern void mqtt_loop();
extern bool mqtt_publish_heart_rate(int bpm);
extern const char* mqtt_get_mode();

// from heart_rate.cpp
extern void heart_rate_init();
extern bool heart_rate_update(float &outBpm);

// from buzzer.cpp
extern void init_buzzer();
extern void buzzer_update();

static unsigned long lastPublish = 0;
static const unsigned long publishInterval = 1000; // max 1/sec

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("ESP32 PulseTracker starting...");

  init_buzzer();
  heart_rate_init();
  mqtt_init();
}

void loop() {
  // keep MQTT/WiFi alive
  mqtt_loop();

  // beep every 5 seconds (non-blocking)
  buzzer_update();

  // heart rate sampling
  float bpm;
  bool newBeat = heart_rate_update(bpm);

  // publish on new beat, rate-limited
  if (newBeat && millis() - lastPublish >= publishInterval) {
    lastPublish = millis();

    int bpmInt = (int)(bpm + 0.5f);

    Serial.print("Mode=");
    Serial.print(mqtt_get_mode());
    Serial.print(" | Publishing BPM: ");
    Serial.println(bpmInt);

    mqtt_publish_heart_rate(bpmInt);
  }
}
