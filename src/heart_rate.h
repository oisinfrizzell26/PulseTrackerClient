/*******************************************************************************
 * Heart Rate Sensor Header - ESP-IDF Implementation
 ******************************************************************************/

#ifndef HEART_RATE_H
#define HEART_RATE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize ADC for heart rate sensor
void heart_rate_init(void);

// Update heart rate reading - returns true if new beat detected
bool heart_rate_update(float *out_bpm);

#ifdef __cplusplus
}
#endif

#endif // HEART_RATE_H
