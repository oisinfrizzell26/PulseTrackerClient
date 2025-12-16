#include <Arduino.h>

// FireBeetle ESP32: GPIO36 is ADC1_CH0 (input only) — good for analog sensors
static const int SENSOR_PIN = 36;

// Beat detection tuning
static float baseline = 0;
static const int THRESHOLD = 110;                 // tune this
static const unsigned long REFRACTORY_MS = 600;   // tune this

static unsigned long lastBeatMs = 0;
static float bpmSmoothed = 0;

// sampling pacing
static const unsigned long SAMPLE_MS = 10;        // ~100Hz
static unsigned long lastSampleMs = 0;

void heart_rate_init() {
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);
  baseline = analogRead(SENSOR_PIN);
}

// Returns true if a *new beat* was detected and outBpm updated
bool heart_rate_update(float &outBpm) {
  unsigned long now = millis();
  if (now - lastSampleMs < SAMPLE_MS) return false;
  lastSampleMs = now;

  int raw = analogRead(SENSOR_PIN);

  // baseline tracks slowly
  baseline = 0.95f * baseline + 0.05f * raw;

  int delta = raw - (int)baseline;
  if (delta < 0) delta = -delta;

  if (delta > THRESHOLD && (now - lastBeatMs) > REFRACTORY_MS) {
    if (lastBeatMs != 0) {
      unsigned long ibi = now - lastBeatMs;
      float bpm = 60000.0f / (float)ibi;

      if (bpmSmoothed == 0) bpmSmoothed = bpm;
      bpmSmoothed = 0.8f * bpmSmoothed + 0.2f * bpm;

      outBpm = bpmSmoothed;
      lastBeatMs = now;
      return true;
    }
    lastBeatMs = now;
  }

  return false;
}
