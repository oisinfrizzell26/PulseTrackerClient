#include <Arduino.h>

static const int BUZZER_PIN = 18;

// LEDC config
static const int BUZZER_CHANNEL = 0;
static const int BUZZER_RES_BITS = 8;
static const int BUZZER_FREQ_HZ = 2000;

// Timing
static const unsigned long BUZZ_INTERVAL_MS = 5000;  // every 5 seconds
static const unsigned long BUZZ_ON_MS = 250;         // beep duration

static unsigned long lastBuzzStart = 0;
static bool buzzing = false;
static unsigned long buzzOnAt = 0;

void init_buzzer() {
  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ_HZ, BUZZER_RES_BITS);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWriteTone(BUZZER_CHANNEL, 0); // off
}

void buzzer_update() {
  unsigned long now = millis();

  // Start a new beep every 5 seconds
  if (!buzzing && (now - lastBuzzStart >= BUZZ_INTERVAL_MS)) {
    lastBuzzStart = now;
    buzzing = true;
    buzzOnAt = now;

    ledcWriteTone(BUZZER_CHANNEL, BUZZER_FREQ_HZ);
  }

  // Stop after BUZZ_ON_MS
  if (buzzing && (now - buzzOnAt >= BUZZ_ON_MS)) {
    buzzing = false;
    ledcWriteTone(BUZZER_CHANNEL, 0); // off
  }
}
