/**
 * @file AVR32DB28 LED driver.
 */
#ifndef LED_H_
#define LED_H_

#include <stdbool.h>

/**
 * @brief Enumeration of LED IDs.
 */
typedef enum
{
    LED_RED,   ///< Red LED (PC0).
    LED_GREEN, ///< Green LED (PC1).
    LED_BLUE,  ///< Blue LED (PC2).
} led_id_t;

/**
 * @brief Initialize all LEDs.
 */
void led_init(void);

/**
 * @brief Get LED state.
 *
 * @param[in] led LED ID.
 *
 * @return True if the LED is on, false otherwise.
 */
bool led_read(led_id_t led);

/**
 * @brief Set LED state.
 *
 * @param[in] led LED ID.
 * @param[in] state LED state (true = on, false = off).
 */
void led_write(led_id_t led, bool state);

/**
 * @brief Toggle LED.
 *
 * @param[in] led LED ID.
 */
void led_toggle(led_id_t led);

#endif /** LED_H_ */
