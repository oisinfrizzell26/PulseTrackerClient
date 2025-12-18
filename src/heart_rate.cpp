#include "heart_rate.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "HEART_RATE";

#define HEART_RATE_PIN ADC1_CHANNEL_0  // GPIO36
#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64
#define THRESHOLD       1500  // Threshold for beat detection (between 142mV and 3129mV)
#define MIN_INTERVAL_MS 300   // Minimum 300ms between beats (200 BPM max)
#define MAX_INTERVAL_MS 2000  // Maximum 2000ms between beats (30 BPM min)
#define REQUIRED_BEATS  3     // Need 3 beats for stable reading

static esp_adc_cal_characteristics_t *adc_chars;
static int current_bpm = 0;
static bool sensor_valid = false;
static uint32_t last_beat_time = 0;
static bool last_state = false;
static TaskHandle_t heart_rate_task_handle = NULL;
static volatile bool beat_detected = false;

// Beat detection variables
static uint32_t beat_times[10] = {0};
static int beat_index = 0;
static int beat_count = 0;

// Signal smoothing
static uint32_t smoothed_voltage = 0;

static uint32_t read_adc_voltage(void) {
    uint32_t adc_reading = 0;
    
    // Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += adc1_get_raw(HEART_RATE_PIN);
    }
    adc_reading /= NO_OF_SAMPLES;
    
    // Convert to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    
    // Smooth the signal - exponential moving average
    if (smoothed_voltage == 0) {
        smoothed_voltage = voltage;
    } else {
        smoothed_voltage = (smoothed_voltage * 7 + voltage * 3) / 10;
    }
    
    return smoothed_voltage;
}

// Heart rate monitoring task
static void heart_rate_task(void *pvParameters) {
    while (1) {
        uint32_t voltage = read_adc_voltage();
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Simple threshold detection
        bool current_state = (voltage > THRESHOLD);
        
        // Detect rising edge (beat)
        if (current_state && !last_state) {
            uint32_t interval = current_time - last_beat_time;
            
            // Validate interval to filter noise
            if (last_beat_time > 0 && interval >= MIN_INTERVAL_MS && interval <= MAX_INTERVAL_MS) {
                beat_times[beat_index] = interval;
                beat_index = (beat_index + 1) % 10;
                if (beat_count < 10) beat_count++;
                
                // Only update BPM if we have enough beats
                if (beat_count >= REQUIRED_BEATS) {
                    // Calculate average BPM from recent beats
                    uint32_t avg_interval = 0;
                    for (int i = 0; i < beat_count; i++) {
                        avg_interval += beat_times[i];
                    }
                    avg_interval /= beat_count;
                    current_bpm = 60000 / avg_interval;
                    beat_detected = true;
                }
            }
            
            last_beat_time = current_time;
        }
        
        last_state = current_state;
        
        // Reset BPM if no beat detected for too long
        if (current_time - last_beat_time > 5000) {
            current_bpm = 0;
            beat_count = 0;
            smoothed_voltage = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // Check every 50ms
    }
}

void heart_rate_init(void) {
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(HEART_RATE_PIN, ADC_ATTEN_DB_12);
    
    // Characterize ADC
    adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    
    // Create heart rate monitoring task
    xTaskCreate(heart_rate_task, "heart_rate_task", 4096, NULL, 5, &heart_rate_task_handle);
    
    sensor_valid = true;
}

int heart_rate_get_bpm(void) {
    return current_bpm;
}

bool heart_rate_is_valid(void) {
    return sensor_valid && (current_bpm > 0);
}

bool heart_rate_update(float *bpm_out) {
    if (bpm_out) {
        *bpm_out = (float)current_bpm;
    }

    bool event = beat_detected;
    beat_detected = false;
    return event;
}

uint32_t heart_rate_read_voltage_debug(void) {
    return read_adc_voltage();
}
