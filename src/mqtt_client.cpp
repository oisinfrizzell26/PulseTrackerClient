#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ---------- WiFi ----------
static const char* ssid     = "James’s iPhone";
static const char* password = "jamesdoherty";

// ---------- MQTT ----------
static const char* mqtt_server = "172.20.10.3";
static const uint16_t mqtt_port = 1883;

static const char* topic_mode       = "pulsetracker/mode";
static const char* topic_heart_rate = "pulsetracker/heartRate";

static WiFiClient espClient;
static PubSubClient client(espClient);

static String currentMode = "unknown";

// ---------- MQTT callback ----------
static void mqttCallback(char* topic, byte* payload, unsigned int length) {
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
static void setupWiFi() {
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
static void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");

    String clientId = "pulsetracker_esp32_";
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

// --------- Public API ----------
void mqtt_init() {
  setupWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void mqtt_loop() {
  // keep WiFi up
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi down, reconnecting...");
    setupWiFi();
    return;
  }

  // keep MQTT up
  if (!client.connected()) reconnectMQTT();
  client.loop();
}

bool mqtt_publish_heart_rate(int bpm) {
  if (!client.connected()) return false;

  char payload[16];
  snprintf(payload, sizeof(payload), "%d", bpm);
  return client.publish(topic_heart_rate, payload);
}

const char* mqtt_get_mode() {
  return currentMode.c_str();
}
