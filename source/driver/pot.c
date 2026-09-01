/**
 * @file AVR32DB28 potentiometer driver implementation details.
 */
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/adc.h"
#include "driver/pot.h"

#define POT_MAX 4095U    // Largest reading a 12-bit conversion gives, i.e. a fully turned pot.
#define PERCENT_MAX 100U // Full scale in percent.

// -----------------------------------------------------------------------------
void pot_init(void) { adc_init(); }

// -----------------------------------------------------------------------------
uint16_t pot_read(const pot_id_t pot)
{
    // Read the specified potentiometer, or return 0 if unknown.
    switch (pot)
    {
        case POT1:
            return adc_read(ADC_POT1);
        case POT2:
            return adc_read(ADC_POT2);
        default:
            return 0U;
    }
}

// -----------------------------------------------------------------------------
uint16_t pot_read_mv(const pot_id_t pot)
{
    // Read the specified potentiometer, or return 0 if unknown.
    switch (pot)
    {
        case POT1:
            return adc_read_mv(ADC_POT1);
        case POT2:
            return adc_read_mv(ADC_POT2);
        default:
            return 0U;
    }
}

// -----------------------------------------------------------------------------
uint8_t pot_read_percent(const pot_id_t pot)
{
    // Scale the reading to percent in 32 bits to avoid overflow.
    return (uint8_t)(((uint32_t)pot_read(pot) * PERCENT_MAX) / POT_MAX);
}
