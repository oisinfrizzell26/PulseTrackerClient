#include "led.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define RED_LED_PIN GPIO_NUM_5   // IO5 - Red LED
#define GREEN_LED_PIN GPIO_NUM_19 // IO19 - Green LED

static const char *TAG = "LED";

void led_init(void) {
    ESP_LOGI(TAG, "Initializing LEDs on GPIO %d (RED) and GPIO %d (GREEN)", RED_LED_PIN, GREEN_LED_PIN);
    
    // Reset pins first
    gpio_reset_pin(RED_LED_PIN);
    gpio_reset_pin(GREEN_LED_PIN);
    
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << RED_LED_PIN) | (1ULL << GREEN_LED_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIOs: %s", esp_err_to_name(ret));
        return;
    }
    
    // Start with both LEDs off
    gpio_set_level(RED_LED_PIN, 0);
    gpio_set_level(GREEN_LED_PIN, 0);
    
    ESP_LOGI(TAG, "LEDs initialized successfully - Red: GPIO %d, Green: GPIO %d", RED_LED_PIN, GREEN_LED_PIN);
}

void red_led_on(void) {
    gpio_set_level(RED_LED_PIN, 1);
    ESP_LOGI(TAG, "RED LED ON - GPIO %d", RED_LED_PIN);
}

void red_led_off(void) {
    gpio_set_level(RED_LED_PIN, 0);
    ESP_LOGI(TAG, "RED LED OFF - GPIO %d", RED_LED_PIN);
}

void green_led_on(void) {
    gpio_set_level(GREEN_LED_PIN, 1);
    ESP_LOGI(TAG, "GREEN LED ON - GPIO %d", GREEN_LED_PIN);
}

void green_led_off(void) {
    gpio_set_level(GREEN_LED_PIN, 0);
    ESP_LOGI(TAG, "GREEN LED OFF - GPIO %d", GREEN_LED_PIN);
}