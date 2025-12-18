#ifndef HR_SESSION_H
#define HR_SESSION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialise HR session handler (creates background task) */
void hr_session_init(void);

/* Start a heart-rate capture session for a lap (runs ~5s then notifies MAX) */
void hr_session_start(uint8_t lap_number);

/* Cancel any in-progress HR session (used on disconnect/stop) */
void hr_session_cancel(void);

#ifdef __cplusplus
}
#endif

#endif /* HR_SESSION_H */
