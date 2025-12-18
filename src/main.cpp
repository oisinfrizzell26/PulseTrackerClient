/*******************************************************************************
 * ESP32 PulseTracker - Main Application
 * ESP-IDF Framework with FreeRTOS Tasks
 *
 * Features:
 * - WiFi + MQTT for cloud connectivity
 * - BLE client to receive workout data from MAX32655
 * - Heart rate sensor via ADC
 * - Buzzer feedback
 ******************************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "app_mqtt.h"
#include "ble_client.h"
#include "heart_rate.h"
#include "hr_session.h"
#include "buzzer.h"

static const char *TAG = "MAIN";

// Heart rate publish rate limiting
static const uint32_t PUBLISH_INTERVAL_MS = 1000;  // max 1/sec

/*******************************************************************************
 * Heart Rate Task
 * Samples ADC and publishes BPM over MQTT
 ******************************************************************************/
static void heart_rate_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Heart rate task started");

    TickType_t last_publish = 0;

    while (1) {
        float bpm;
        bool new_beat = heart_rate_update(&bpm);

        TickType_t now = xTaskGetTickCount();
        uint32_t elapsed = (now - last_publish) * portTICK_PERIOD_MS;

        if (new_beat && elapsed >= PUBLISH_INTERVAL_MS) {
            last_publish = now;

            int bpm_int = (int)(bpm + 0.5f);

            ESP_LOGI(TAG, "Mode=%s | BPM=%d | BLE=%s",
                     mqtt_get_mode(),
                     bpm_int,
                     ble_client_is_connected() ? "connected" : "scanning");

            mqtt_publish_heart_rate(bpm_int);
        }

        // Small delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/*******************************************************************************
 * Buzzer Task
 * Periodic beep feedback
 ******************************************************************************/
static void buzzer_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Buzzer task started");

    while (1) {
        buzzer_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*******************************************************************************
 * App Main Entry Point
 ******************************************************************************/
extern "C" void app_main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("   ESP32 PulseTracker v2.0 (ESP-IDF)\n");
    printf("   WiFi + MQTT + BLE + Heart Rate\n");
    printf("========================================\n\n");

    // Initialize peripherals
    ESP_LOGI(TAG, "Initializing buzzer...");
    buzzer_init();

    ESP_LOGI(TAG, "Initializing heart rate sensor...");
    heart_rate_init();

    ESP_LOGI(TAG, "Initializing HR session manager...");
    hr_session_init();

    // Initialize WiFi and MQTT (blocks until WiFi connected)
    ESP_LOGI(TAG, "Initializing WiFi and MQTT...");
    mqtt_init();

    // Initialize BLE client (starts scanning for MAX32655)
    ESP_LOGI(TAG, "Initializing BLE client...");
    ble_client_init();

    // Create FreeRTOS tasks
    xTaskCreate(heart_rate_task, "heart_rate", 4096, NULL, 5, NULL);
    // Buzzer task disabled - using MQTT-triggered buzzer only
    // xTaskCreate(buzzer_task, "buzzer", 2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "All systems initialized!");
    printf("\n========================================\n");
    printf("   Ready! Scanning for MAX32655...\n");
    printf("========================================\n\n");
}
