#ifndef HEART_RATE_H
#define HEART_RATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize heart rate sensor on GPIO36
void heart_rate_init(void);

// Get current heart rate (BPM)
int heart_rate_get_bpm(void);

// Check if heart rate is valid
bool heart_rate_is_valid(void);

// Debug function to read raw voltage
uint32_t heart_rate_read_voltage_debug(void);

#ifdef __cplusplus
}
#endif

#endif 
