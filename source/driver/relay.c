/**
 * @file AVR32DB28 relay driver implementation details.
 */

#include <stdbool.h>

#include "arch/avr/hw_platform.h"
#include "driver/relay.h"

#define PORT PORTA   // Port register for the relay.
#define RELAY_PIN 7U // Relay pin (PORTA7).

// -----------------------------------------------------------------------------
void relay_init(void)
{
    // Configure relay as output.
    PORTA.DIR |= (1U << RELAY_PIN);
}

// -----------------------------------------------------------------------------
bool relay_read(void)
{
    // Read the relay state, return true if on, false if off.
    return (bool)(PORTA.OUT & (1U << RELAY_PIN));
}

// -----------------------------------------------------------------------------
void relay_write(const bool state)
{
    // Set relay state as specified.
    if (state) { PORTA.OUTSET = (1U << RELAY_PIN); }
    else { PORTA.OUTCLR = (1U << RELAY_PIN); }
}

// -----------------------------------------------------------------------------
void relay_toggle(void)
{
    // Toggle the relay.
    PORTA.OUTTGL = (1U << RELAY_PIN);
}
