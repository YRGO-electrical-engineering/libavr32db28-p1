/**
 * @file AVR32DB28 potentiometer driver.
 */
#ifndef POT_H_
#define POT_H_

#include <stdint.h>

/**
 * @brief Enumeration of potentiomer IDs.
 */
typedef enum
{
    POT1, ///< Potentiometer 1 (PD1).
    POT2, ///< Potentiometer 2 (PD3).
} pot_id_t;

/**
 * @brief Initialize potentiometers.
 */
void pot_init(void);

/**
 * @brief Read potentiometer.
 *
 * @param[in] pot Potentiometer to read.
 *
 * @return Measured potentiometer voltage as a 12-bit value, from 0 at 0 V to 4095 at 5 V.
 *         An unknown pot ID reads as 0.
 */
uint16_t pot_read(pot_id_t pot);

/**
 * @brief Read potentiometer voltage in mV.
 *
 * @param[in] pot Potentiometer to read.
 *
 * @return Potentiometer voltage in mV. An unknown pot ID reads as 0.
 */
uint16_t pot_read_mv(pot_id_t pot);

/**
 * @brief Read potentiometer voltage in percent.
 *
 * @param[in] pot Potentiometer to read.
 *
 * @return Measured potentiometer voltage in percent, from 0 % at 0 V to 100 % at 5 V.
 *         An unknown pot ID reads as 0 %.
 */
uint8_t pot_read_percent(pot_id_t pot);

#endif /** POT_H_ */
