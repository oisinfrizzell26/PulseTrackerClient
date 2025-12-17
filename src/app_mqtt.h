/*******************************************************************************
 * MQTT Client Header - ESP-IDF Implementation
 ******************************************************************************/

#ifndef APP_MQTT_CLIENT_H
#define APP_MQTT_CLIENT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize WiFi and MQTT
void mqtt_init(void);

// Publish heart rate BPM value
bool mqtt_publish_heart_rate(int bpm);

// Publish workout JSON data from BLE
bool mqtt_publish_workout_data(const char* json_data);

// Get current mode string
const char* mqtt_get_mode(void);

// Check if MQTT is connected
bool mqtt_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // APP_MQTT_CLIENT_H
