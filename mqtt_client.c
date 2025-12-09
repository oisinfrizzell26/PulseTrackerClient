#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>
#include <signal.h>

#define MQTT_BROKER "alderaan.software-engineering.ie"
#define MQTT_PORT 1883
#define CLIENT_ID "pulsetracker_client_oisin"
#define TOPIC_MODE "pulsetracker/oisin/mode"

static int running = 1;

void handle_signal(int sig) {
    printf("\n🛑 Shutting down...\n");
    running = 0;
}

void on_connect(struct mosquitto *mosq, void *obj, int result) {
    if (result == 0) {
        printf("✅ Connected to MQTT broker\n");
        mosquitto_subscribe(mosq, NULL, TOPIC_MODE, 0);
        printf("🔔 Subscribed to: %s\n", TOPIC_MODE);
    } else {
        printf("❌ Connection failed: %d\n", result);
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    if (strcmp(message->topic, TOPIC_MODE) == 0) {
        char *payload = (char*)message->payload;
        int mode = atoi(payload);
        
        switch(mode) {
            case 1:
                printf("📱 Mode set to: Fitness Mode\n");
                break;
            case 2:
                printf("📱 Mode set to: Lap Mode\n");
                break;
            default:
                printf("📱 Mode set to: %s\n", payload);
                break;
        }
    }
}

int main() {
    printf("🚀 PulseTracker MQTT Client\n");
    printf("==============================\n");
    printf("🎯 Connecting to: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    printf("👂 Listening for mode commands...\n\n");
    
    signal(SIGINT, handle_signal);
    
    mosquitto_lib_init();
    
    struct mosquitto *mosq = mosquitto_new(CLIENT_ID, true, NULL);
    if (!mosq) {
        printf("❌ Failed to create client\n");
        return 1;
    }
    
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    
    if (mosquitto_connect(mosq, MQTT_BROKER, MQTT_PORT, 60) != MOSQ_ERR_SUCCESS) {
        printf("❌ Failed to connect to broker\n");
        return 1;
    }
    
    mosquitto_loop_start(mosq);
    
    printf("✅ Client ready! Press Ctrl+C to exit\n\n");
    
    while (running) {
        sleep(1);
    }
    
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    
    printf("✅ Client shutdown complete\n");
    return 0;
}