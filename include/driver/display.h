/**
 * @file AVR32DB28 7-segment display driver.
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize the display.
 *
 * @return True on success, false if no timer is available for the display.
 */
bool display_init(void);

/**
 * @brief Clear the display.
 */
void display_clear(void);

/**
 * @brief Read display value.
 *
 * @return Current display value (0 - 99), or -1 if the display is blank.
 */
int8_t display_read(void);

/**
 * @brief Write display value.
 *
 * @param[in] value Display value (0 - 99).
 *
 * @note The display shows decimal digits only, and a value above 99 is ignored.
 */
void display_write(uint8_t value);

/**
 * @brief Update the display.
 *
 * @note This function should be called once every loop iteration.
 */
void display_update(void);

/**
 * @brief Show/hide decimal point.
 *
 *        When shown, a value like 37 is displayed as 3.7.
 *
 * @param[in] show True to show the decimal point, false to hide it.
 */
void display_show_dp(bool show);

#endif /** DISPLAY_H_ */
