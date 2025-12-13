#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ---------- WiFi ----------
const char* ssid     = "James’s iPhone";
const char* password = "jamesdoherty";

// ---------- MQTT ----------
const char* mqtt_server = "172.20.10.3";   // laptop/broker IP (Mosquitto)
const uint16_t mqtt_port = 1883;

// Use a consistent topic namespace (matches your C client style)
const char* topic_mode       = "pulsetracker/mode";
const char* topic_heart_rate = "pulsetracker/heartRate";

String currentMode = "unknown";

WiFiClient espClient;
PubSubClient client(espClient);

// ---------- Heart-rate sensor ----------
const int SENSOR_PIN = 36;

// Beat detection
float baseline = 0;
const int THRESHOLD = 110;                 // tune this
const unsigned long REFRACTORY_MS = 600;   // tune this

unsigned long lastBeatMs = 0;
float bpmSmoothed = 0;

// Publish timing
unsigned long lastPublish = 0;
const unsigned long publishInterval = 1000; // publish max 1/sec

// ---------- MQTT callback ----------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.println("---- MQTT message received ----");
  Serial.print("Topic: "); Serial.println(topic);
  Serial.print("Payload: "); Serial.println(msg);

  if (String(topic) == topic_mode) {
    currentMode = msg;
    Serial.print("Updated currentMode to: ");
    Serial.println(currentMode);
  }
  Serial.println("-------------------------------");
}

// ---------- WiFi connect ----------
void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("Failed WiFi. status=");
    Serial.println(WiFi.status());
  }
}

// ---------- MQTT reconnect ----------
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");

    String clientId = "pulsetracker_esp32_oisin_";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      client.subscribe(topic_mode);
      Serial.print("Subscribed to: ");
      Serial.println(topic_mode);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 2s");
      delay(2000);
    }
  }
}

// Returns true if a *new beat* was detected and bpmSmoothed updated
bool updateBpmFromSensor(float &outBpm) {
  int raw = analogRead(SENSOR_PIN);

  // baseline tracks slowly
  baseline = 0.95f * baseline + 0.05f * raw;

  int delta = raw - (int)baseline;
  if (delta < 0) delta = -delta;

  unsigned long now = millis();

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

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("ESP32 PulseTracker starting...");

  // ADC config (ESP32)
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);
  baseline = analogRead(SENSOR_PIN);

  setupWiFi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void loop() {
  // keep WiFi up
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi down, reconnecting...");
    setupWiFi();
    delay(1000);
    return;
  }

  // keep MQTT up
  if (!client.connected()) reconnectMQTT();
  client.loop();

  // sample sensor ~100Hz
  float bpm;
  bool newBeat = updateBpmFromSensor(bpm);
  delay(10);

  // publish when we have a value, but rate-limit publishing
  if (newBeat && millis() - lastPublish >= publishInterval) {
    lastPublish = millis();

    // keep payload simple for your C program (atoi-friendly):
    // e.g. "72" instead of JSON
    char payload[16];
    snprintf(payload, sizeof(payload), "%d", (int)(bpm + 0.5f));

    Serial.print("Mode="); Serial.print(currentMode);
    Serial.print(" | Publishing BPM: ");
    Serial.println(payload);

    client.publish(topic_heart_rate, payload);
  }
}
