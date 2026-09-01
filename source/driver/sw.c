/**
 * @file AVR32DB28 switch driver implementation details.
 */
#include <stdbool.h>
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/sw.h"

#define SW_PIN1 3U // Switch PIN 1 (PORTC3).
#define SW_PIN2 0U // Switch PIN 2 (PORTF0).
#define SW_PIN3 1U // Switch PIN 3 (PORTF1).

// -----------------------------------------------------------------------------
void sw_init(void)
{
    // Configure the switches as inputs.
    PORTC.DIR &= ~(1U << SW_PIN1);
    PORTF.DIR &= ~(1U << SW_PIN2);
    PORTF.DIR &= ~(1U << SW_PIN3);

    // Each switch connects its pin to +5 V when pressed, and an external resistor ties the pin
    // to ground when released. The internal pull-ups are therefore switched off: one would
    // fight the external resistor and leave the pin stuck between high and low.
    PORTC.PIN3CTRL &= ~PORT_PULLUPEN_bm;
    PORTF.PIN0CTRL &= ~PORT_PULLUPEN_bm;
    PORTF.PIN1CTRL &= ~PORT_PULLUPEN_bm;
}

// -----------------------------------------------------------------------------
bool sw_read(const switch_id_t sw)
{
    // The switches are connected to +5 V, so a pressed switch reads high.
    switch (sw)
    {
        case SW1:
            return (bool)(PORTC.IN & (1U << SW_PIN1));
        case SW2:
            return (bool)(PORTF.IN & (1U << SW_PIN2));
        case SW3:
            return (bool)(PORTF.IN & (1U << SW_PIN3));
        default:
            return false;
    }
}
