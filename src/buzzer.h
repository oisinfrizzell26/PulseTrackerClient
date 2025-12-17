/*******************************************************************************
 * Buzzer Header - ESP-IDF Implementation
 ******************************************************************************/

#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize LEDC for buzzer
void buzzer_init(void);

// Non-blocking update - call in main loop
void buzzer_update(void);

// Blocking beep for specified duration
void buzzer_beep(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif // BUZZER_H
