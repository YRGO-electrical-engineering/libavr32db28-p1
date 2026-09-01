/**
 * @file AVR32DB28 LED driver implementation details.
 */
#include <stdbool.h>
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/led.h"

#define PORT PORTC        // Port register for all LEDs.
#define PWM_TIMER TCA0    // Timer generating the PWM for all LEDs.
#define PERCENT_MAX 100U  // Maximum duty cycle (fully on).
#define PWM_PERIOD 99U    // Counter top, chosen so that a compare value is a percentage.
#define PWM_PRESCALER 16U // Clock divisor, giving a period far too fast for the eye to see.

// -----------------------------------------------------------------------------
static uint8_t get_compare_enable(const led_id_t led)
{
    // Return the bit connecting the LED's pin to the timer instead of the output register.
    switch (led)
    {
        case LED_RED:
            return TCA_SINGLE_CMP0EN_bm;
        case LED_GREEN:
            return TCA_SINGLE_CMP1EN_bm;
        default:
            return TCA_SINGLE_CMP2EN_bm;
    }
}

// -----------------------------------------------------------------------------
static void set_duty_cycle(const led_id_t led, const uint8_t percent)
{
    // Set how much of each period the LED is lit; the counter tops out at 99, so a compare
    // value of 100 never matches and leaves the LED fully on.
    switch (led)
    {
        case LED_RED:
            PWM_TIMER.SINGLE.CMP0 = percent;
            break;
        case LED_GREEN:
            PWM_TIMER.SINGLE.CMP1 = percent;
            break;
        default:
            PWM_TIMER.SINGLE.CMP2 = percent;
            break;
    }
}

// -----------------------------------------------------------------------------
static uint8_t get_duty_cycle(const led_id_t led)
{
    // Return how much of each period the LED is lit.
    switch (led)
    {
        case LED_RED:
            return (uint8_t)PWM_TIMER.SINGLE.CMP0;
        case LED_GREEN:
            return (uint8_t)PWM_TIMER.SINGLE.CMP1;
        default:
            return (uint8_t)PWM_TIMER.SINGLE.CMP2;
    }
}

// -----------------------------------------------------------------------------
static void release_pin(const led_id_t led)
{
    // Hand the pin back from the timer, so that the output register drives it again.
    PWM_TIMER.SINGLE.CTRLB &= ~get_compare_enable(led);
}

// -----------------------------------------------------------------------------
void led_init(void)
{
    // Configure LEDs as outputs.
    PORT.DIR |= (1U << LED_RED) | (1U << LED_GREEN) | (1U << LED_BLUE);

    // Route the timer's outputs to the LEDs, which sit on port C rather than the default port A.
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTC_gc;

    // Count to 99 and start over, so that a compare value reads as a percentage.
    PWM_TIMER.SINGLE.PER = PWM_PERIOD;

    // Generate single slope PWM, with every LED still driven by the output register for now.
    PWM_TIMER.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;

    // Run the timer slowly enough to keep it out of the way, but far above what the eye sees.
    PWM_TIMER.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm;
}

// -----------------------------------------------------------------------------
bool led_read(const led_id_t led)
{
    // A dimmed LED is lit whenever its duty cycle is above zero.
    if (PWM_TIMER.SINGLE.CTRLB & get_compare_enable(led)) { return 0U != get_duty_cycle(led); }

    // Otherwise read the LED state, return true if on, false if off.
    return (bool)(PORT.OUT & (1U << led));
}

// -----------------------------------------------------------------------------
void led_write(const led_id_t led, const bool state)
{
    // Take the pin back from the timer, since the LED is now switched rather than dimmed.
    release_pin(led);

    // Set LED state as specified.
    if (state) { PORT.OUTSET = (1U << led); }
    else { PORT.OUTCLR = (1U << led); }
}

// -----------------------------------------------------------------------------
void led_toggle(const led_id_t led)
{
    // Take the pin back from the timer, since the LED is now switched rather than dimmed.
    release_pin(led);

    // Toggle the LED.
    PORT.OUTTGL = (1U << led);
}

// -----------------------------------------------------------------------------
void led_pwm(const led_id_t led, const uint8_t percent)
{
    // Check the duty cycle, do nothing if invalid.
    if (PERCENT_MAX < percent) { return; }

    // Set the brightness, then let the timer drive the pin until the LED is switched again.
    set_duty_cycle(led, percent);
    PWM_TIMER.SINGLE.CTRLB |= get_compare_enable(led);
}
