/**
 * @file AVR32DB28 temperature sensor driver.
 */
#ifndef TEMP_H_
#define TEMP_H_

#include <stdint.h>

/**
 * @brief Initialize temperature sensor.
 */
void temp_init(void);

/**
 * @brief Read temperature sensor.
 *
 * @return Temperature in degrees Celsius.
 */
int16_t temp_read(void);

#endif /** TEMP_H_ */