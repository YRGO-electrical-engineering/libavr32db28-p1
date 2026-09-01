/**
 * @file AVR32DB28 ADC driver implementation details.
 */
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/adc.h"

#define PORT PORTD // Port register for the analog inputs.

#define MUXPOS_NONE 0xFFU // No analog input, i.e. the channel is unknown.

#define SAMPLE_LENGTH 8U         // Extra cycles spent charging the sample capacitor.
#define CONVERSION_TIMEOUT 1000U // Times the result is checked for before giving up.
#define NO_VALUE 0U              // Returned when no conversion could be made.

#define VDD_MV 5000U // Supply voltage in mV, i.e. what a full reading means.
#define STEPS 4096U  // Steps a 12-bit conversion is divided into.

// -----------------------------------------------------------------------------
static uint8_t get_muxpos(const adc_channel_t channel)
{
    // Return the analog input the channel is wired to, or none if the channel is unknown.
    switch (channel)
    {
        case ADC_POT1:
            return ADC_MUXPOS_AIN1_gc;
        case ADC_POT2:
            return ADC_MUXPOS_AIN3_gc;
        case ADC_TEMP:
            return ADC_MUXPOS_AIN4_gc;
        case ADC_JOYSTICK_Y:
            return ADC_MUXPOS_AIN5_gc;
        case ADC_JOYSTICK_X:
            return ADC_MUXPOS_AIN6_gc;
        default:
            return MUXPOS_NONE;
    }
}

// -----------------------------------------------------------------------------
void adc_init(void)
{
    // Switch off the digital input buffers; these pins carry voltages, not logic levels.
    PORT.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORT.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORT.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORT.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORT.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;

    // Measure against the supply voltage, so that a full reading means 5 V.
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;

    // Clock the ADC at 1 MHz and sample for longer than the default, the pots being 10 kohm.
    ADC0.CTRLC    = ADC_PRESC_DIV4_gc;
    ADC0.SAMPCTRL = SAMPLE_LENGTH;

    // Enable the ADC at the full resolution the device offers.
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;
}

// -----------------------------------------------------------------------------
uint16_t adc_read(const adc_channel_t channel)
{
    // Look up the analog input, terminate if the channel is unknown.
    const uint8_t muxpos = get_muxpos(channel);
    if (MUXPOS_NONE == muxpos) { return NO_VALUE; }

    // Select the input and start a conversion.
    ADC0.MUXPOS  = muxpos;
    ADC0.COMMAND = ADC_STCONV_bm;

    // Wait for the result, but give up eventually so a silent ADC can't stop the program.
    for (uint16_t i = 0U; i < CONVERSION_TIMEOUT; ++i)
    {
        if (ADC0.INTFLAGS & ADC_RESRDY_bm)
        {
            // Read the result before lowering the flag; reading is itself one way to clear it.
            const uint16_t result = ADC0.RES;
            ADC0.INTFLAGS         = ADC_RESRDY_bm;
            return result;
        }
    }
    return NO_VALUE;
}

// -----------------------------------------------------------------------------
uint16_t adc_read_mv(const adc_channel_t channel)
{
    // Scale the reading to millivolts, in 32 bits so that the multiplication can't overflow.
    return (uint16_t)(((uint32_t)adc_read(channel) * VDD_MV) / STEPS);
}
