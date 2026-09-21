/**
 * @file AVR32DB28 LED driver implementation details.
 */
#include <stdbool.h>

#include "arch/avr/hw_platform.h"
#include "driver/led.h"

#define PORT PORTC // Port register for all LEDs.

// -----------------------------------------------------------------------------
void led_init(void)
{
    // Configure LEDs as outputs.
    PORT.DIR |= (1U << LED_RED) | (1U << LED_GREEN) | (1U << LED_BLUE);
}

// -----------------------------------------------------------------------------
bool led_read(const led_id_t led)
{
    // Read the LED state, return true if on, false if off.
    return (bool)(PORT.OUT & (1U << led));
}

// -----------------------------------------------------------------------------
void led_write(const led_id_t led, const bool state)
{
    // Set LED state as specified.
    if (state) { PORT.OUTSET = (1U << led); }
    else { PORT.OUTCLR = (1U << led); }
}

// -----------------------------------------------------------------------------
void led_toggle(const led_id_t led)
{
    // Toggle the LED.
    PORT.OUTTGL = (1U << led);
}
