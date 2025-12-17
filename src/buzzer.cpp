/*******************************************************************************
 * Buzzer - ESP-IDF Implementation
 * Uses LEDC PWM to drive buzzer on GPIO18
 ******************************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "buzzer.h"

static const char *TAG = "BUZZER";

// Buzzer configuration
#define BUZZER_GPIO         GPIO_NUM_18
#define BUZZER_LEDC_TIMER   LEDC_TIMER_0
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define BUZZER_FREQ_HZ      2000
#define BUZZER_RESOLUTION   LEDC_TIMER_8_BIT

// Timing
static const unsigned long BUZZ_INTERVAL_MS = 5000;  // every 5 seconds
static const unsigned long BUZZ_ON_MS = 250;         // beep duration

static unsigned long last_buzz_start = 0;
static bool buzzing = false;
static unsigned long buzz_on_at = 0;

// Get current time in ms
static unsigned long millis(void)
{
    return (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void buzzer_init(void)
{
    // Configure LEDC timer
    ledc_timer_config_t timer_cfg = {
        .speed_mode = BUZZER_LEDC_MODE,
        .duty_resolution = BUZZER_RESOLUTION,
        .timer_num = BUZZER_LEDC_TIMER,
        .freq_hz = BUZZER_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    // Configure LEDC channel
    ledc_channel_config_t channel_cfg = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = BUZZER_LEDC_MODE,
        .channel = BUZZER_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BUZZER_LEDC_TIMER,
        .duty = 0,  // Start off
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));

    ESP_LOGI(TAG, "Buzzer initialized on GPIO%d", BUZZER_GPIO);
}

static void buzzer_on(void)
{
    // 50% duty cycle for audible tone
    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 128);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

static void buzzer_off(void)
{
    ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

void buzzer_update(void)
{
    unsigned long now = millis();

    // Start a new beep every 5 seconds
    if (!buzzing && (now - last_buzz_start >= BUZZ_INTERVAL_MS)) {
        last_buzz_start = now;
        buzzing = true;
        buzz_on_at = now;
        buzzer_on();
    }

    // Stop after BUZZ_ON_MS
    if (buzzing && (now - buzz_on_at >= BUZZ_ON_MS)) {
        buzzing = false;
        buzzer_off();
    }
}

void buzzer_beep(uint32_t duration_ms)
{
    buzzer_on();
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    buzzer_off();
}
