/*******************************************************************************
 * BLE Client Header - ESP-IDF Implementation
 ******************************************************************************/

#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Callback type for workout data
typedef void (*ble_workout_callback_t)(const char* json_data, uint16_t len);

// Initialize NimBLE BLE client
void ble_client_init(void);

// Check if connected to MAX32655
bool ble_client_is_connected(void);

// Set callback for workout data (optional, MQTT publish is automatic)
void ble_client_set_workout_callback(ble_workout_callback_t callback);

// Send JSON message to MAX (writes RX characteristic), returns true on success
bool ble_client_send_message(const char *msg);

#ifdef __cplusplus
}
#endif

#endif // BLE_CLIENT_H
