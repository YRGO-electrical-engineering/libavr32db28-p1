/**
 * @file AVR32DB28 temperature sensor driver implementation details.
 */
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/adc.h"
#include "driver/temp.h"

#define OFFSET_MV 500    // Sensor output at 0 degrees Celsius, in mV.
#define MV_PER_DEGREE 10 // Sensor output per degree Celsius, in mV.
#define HALF_DEGREE_MV 5 // Half a degree in mV, added before dividing to round to the nearest.

// -----------------------------------------------------------------------------
void temp_init(void) { adc_init(); }

// -----------------------------------------------------------------------------
int16_t temp_read(void)
{
    // The sensor outputs 500 mV at 0 degrees, and 10 mV more for every degree above that.
    const int16_t mv         = (int16_t)adc_read_mv(ADC_TEMP);
    const int16_t above_zero = (int16_t)(mv - OFFSET_MV);

    // Round to the nearest degree rather than always towards zero, so 23.9 reads as 24.
    const int16_t rounding = 0 <= above_zero ? HALF_DEGREE_MV : -HALF_DEGREE_MV;
    return (int16_t)((above_zero + rounding) / MV_PER_DEGREE);
}
