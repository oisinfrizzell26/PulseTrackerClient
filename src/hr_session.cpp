#include "hr_session.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "heart_rate.h"
#include "ble_client.h"

static const char *TAG = "HR_SESSION";

/* How long to collect HR before reporting back to MAX (ms) */
#define HR_CAPTURE_WINDOW_MS 5000

/* Static resources sized for embedded target */
#define HR_QUEUE_LENGTH         4
#define HR_TASK_STACK_WORDS  2048

/* ---------- Command queue ---------- */

typedef enum {
    HR_CMD_START,
    HR_CMD_CANCEL
} hr_cmd_type_t;

typedef struct {
    hr_cmd_type_t type;
    uint8_t lap;
} hr_cmd_t;

static QueueHandle_t hr_cmd_queue = NULL;
static TaskHandle_t hr_task_handle = NULL;
static StaticQueue_t hr_queue_struct;
static uint8_t hr_queue_storage[HR_QUEUE_LENGTH * sizeof(hr_cmd_t)];
static StaticTask_t hr_task_tcb;
static StackType_t hr_task_stack[HR_TASK_STACK_WORDS];

/* ---------- Internal helpers ---------- */

static void hr_task(void *param)
{
    (void)param;

    hr_cmd_t cmd;

    while (1) {
        if (xQueueReceive(hr_cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (cmd.type == HR_CMD_CANCEL) {
            ESP_LOGI(TAG, "HR session cancelled");
            continue;
        }

        /* HR_CMD_START */
        ESP_LOGI(TAG, "HR session started for lap %u", cmd.lap);

        uint32_t start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t last_ms = start_ms;
        int last_bpm = -1;

        while (1) {
            /* Poll sensor; keep latest valid BPM */
            float bpm_f = 0.0f;
            if (heart_rate_update(&bpm_f) && heart_rate_is_valid()) {
                last_bpm = (int)bpm_f;
            }

            /* Exit after capture window or if cancelled */
            uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now_ms - start_ms >= HR_CAPTURE_WINDOW_MS) {
                break;
            }

            /* Non-blocking check for cancel while waiting */
            if (xQueueReceive(hr_cmd_queue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE) {
                if (cmd.type == HR_CMD_CANCEL) {
                    ESP_LOGI(TAG, "HR session cancelled mid-flight");
                    last_bpm = -1;
                    break;
                } else {
                    /* Ignore nested start until current completes */
                    ESP_LOGW(TAG, "Nested HR start ignored");
                }
            }

            vTaskDelay(pdMS_TO_TICKS(50));
            last_ms = now_ms;
        }

        char msg[64];
        int bpm_to_send = (last_bpm > 0) ? last_bpm : 0;
        int len = snprintf(msg, sizeof(msg),
                           "{\"cmd\":\"hr_done\",\"bpm\":%d}", bpm_to_send);
        if (len > 0 && ble_client_send_message(msg)) {
            ESP_LOGI(TAG, "Sent hr_done (bpm=%d)", bpm_to_send);
        } else {
            ESP_LOGW(TAG, "Failed to send hr_done");
        }
    }
}

/* ---------- Public API ---------- */

void hr_session_init(void)
{
    if (hr_cmd_queue != NULL) {
        return;
    }

    hr_cmd_queue = xQueueCreateStatic(
        HR_QUEUE_LENGTH,
        sizeof(hr_cmd_t),
        hr_queue_storage,
        &hr_queue_struct);

    if (hr_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create HR queue");
        return;
    }

    hr_task_handle = xTaskCreateStatic(
        hr_task,
        "hr_session",
        HR_TASK_STACK_WORDS,
        NULL,
        4,
        hr_task_stack,
        &hr_task_tcb);

    if (hr_task_handle == NULL) {
        ESP_LOGE(TAG, "Failed to start HR task");
        hr_cmd_queue = NULL;
    }
}

void hr_session_start(uint8_t lap_number)
{
    if (hr_cmd_queue == NULL) {
        ESP_LOGW(TAG, "HR session not initialised");
        return;
    }

    hr_cmd_t cmd = {
        .type = HR_CMD_START,
        .lap = lap_number
    };
    xQueueSend(hr_cmd_queue, &cmd, 0);
}

void hr_session_cancel(void)
{
    if (hr_cmd_queue == NULL) {
        return;
    }
    hr_cmd_t cmd = { .type = HR_CMD_CANCEL, .lap = 0 };
    xQueueSend(hr_cmd_queue, &cmd, 0);
}
