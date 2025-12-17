/*******************************************************************************
 * Heart Rate Sensor - ESP-IDF Implementation
 * Uses ADC to read analog heart rate sensor on GPIO36 (ADC1_CH0)
 ******************************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "heart_rate.h"

static const char *TAG = "HEART_RATE";

// ADC configuration - GPIO36 is ADC1 Channel 0
#define SENSOR_ADC_UNIT     ADC_UNIT_1
#define SENSOR_ADC_CHANNEL  ADC_CHANNEL_0
#define SENSOR_ADC_ATTEN    ADC_ATTEN_DB_12  // Full range 0-3.3V

// Beat detection tuning
static float baseline = 0;
static const int THRESHOLD = 110;
static const unsigned long REFRACTORY_MS = 600;

static unsigned long last_beat_ms = 0;
static float bpm_smoothed = 0;

// Sampling pacing
static const unsigned long SAMPLE_MS = 10;  // ~100Hz
static unsigned long last_sample_ms = 0;

// ADC handle
static adc_oneshot_unit_handle_t adc_handle = NULL;

// Get current time in ms (using FreeRTOS tick)
static unsigned long millis(void)
{
    return (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void heart_rate_init(void)
{
    // Configure ADC unit
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = SENSOR_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    // Configure ADC channel
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = SENSOR_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, SENSOR_ADC_CHANNEL, &chan_cfg));

    // Read initial baseline
    int raw;
    adc_oneshot_read(adc_handle, SENSOR_ADC_CHANNEL, &raw);
    baseline = (float)raw;

    ESP_LOGI(TAG, "Heart rate sensor initialized (GPIO36, baseline: %.0f)", baseline);
}

bool heart_rate_update(float *out_bpm)
{
    unsigned long now = millis();
    if (now - last_sample_ms < SAMPLE_MS) return false;
    last_sample_ms = now;

    int raw;
    esp_err_t ret = adc_oneshot_read(adc_handle, SENSOR_ADC_CHANNEL, &raw);
    if (ret != ESP_OK) return false;

    // Baseline tracks slowly
    baseline = 0.95f * baseline + 0.05f * (float)raw;

    int delta = raw - (int)baseline;
    if (delta < 0) delta = -delta;

    if (delta > THRESHOLD && (now - last_beat_ms) > REFRACTORY_MS) {
        if (last_beat_ms != 0) {
            unsigned long ibi = now - last_beat_ms;
            float bpm = 60000.0f / (float)ibi;

            if (bpm_smoothed == 0) bpm_smoothed = bpm;
            bpm_smoothed = 0.8f * bpm_smoothed + 0.2f * bpm;

            *out_bpm = bpm_smoothed;
            last_beat_ms = now;
            return true;
        }
        last_beat_ms = now;
    }

    return false;
}
