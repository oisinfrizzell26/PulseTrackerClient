#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize LED GPIOs
 */
void led_init(void);

/**
 * Red LED control
 */
void red_led_on(void);
void red_led_off(void);

/**
 * Green LED control
 */
void green_led_on(void);
void green_led_off(void);

#ifdef __cplusplus
}
#endif

#endif // LED_H