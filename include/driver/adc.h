/**
 * @file AVR32DB28 ADC driver.
 */
#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

/**
 * @brief Enumeration of ADC channels.
 */
typedef enum
{
    ADC_POT1,       ///< Potentiometer 1 (PD1).
    ADC_POT2,       ///< Potentiometer 2 (PD3).
    ADC_TEMP,       ///< Temperature sensor (PD4).
    ADC_JOYSTICK_Y, ///< Joystick, y-axis (PD5).
    ADC_JOYSTICK_X, ///< Joystick, x-axis (PD6).
} adc_channel_t;

/**
 * @brief Initialize ADC.
 */
void adc_init(void);

/**
 * @brief Read ADC channel.
 *
 * @param[in] channel Channel to read.
 *
 * @return Measured voltage as a 12-bit value, from 0 at 0 V to 4095 at 5 V.
 *         An unknown channel reads as 0.
 */
uint16_t adc_read(adc_channel_t channel);

/**
 * @brief Read input voltage on the given channel.
 *
 * @param[in] channel Channel to read.
 *
 * @return Measured voltage in mV. An unknown channel reads as 0.
 */
uint16_t adc_read_mv(adc_channel_t channel);

#endif /** ADC_H_ */
