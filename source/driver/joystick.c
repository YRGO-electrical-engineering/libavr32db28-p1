/**
 * @file AVR32DB28 joystick driver implementation details.
 */
#include <stdbool.h>
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/adc.h"
#include "driver/joystick.h"

#define PORT PORTD    // Port register for the joystick.
#define BUTTON_PIN 7U // Joystick button pin (PORTD7).

#define CENTER 2048    // Reading each axis gives when the joystick is at rest.
#define DEAD_ZONE 1024 // How far an axis has to move before a direction is reported.

// -----------------------------------------------------------------------------
static int16_t get_offset(const adc_channel_t channel)
{
    // Measure how far the axis has moved from the centre, and in which direction.
    return (int16_t)((int16_t)adc_read(channel) - CENTER);
}

// -----------------------------------------------------------------------------
static int16_t get_distance(const int16_t offset)
{
    // Measure how far the axis has moved, whichever way it went.
    return (0 <= offset) ? offset : (int16_t)(-offset);
}

// -----------------------------------------------------------------------------
void joystick_init(void)
{
    // Both axes are analog, so the ADC does the reading.
    adc_init();

    // The button grounds its pin when pressed, so the internal pull-up holds the pin high while
    // it is released.
    PORT.DIR &= ~(1U << BUTTON_PIN);
    PORT.PIN7CTRL |= PORT_PULLUPEN_bm;
}

// -----------------------------------------------------------------------------
joystick_dir_t joystick_read(void)
{
    // Measure both axes.
    const int16_t x = get_offset(ADC_JOYSTICK_X);
    const int16_t y = get_offset(ADC_JOYSTICK_Y);

    // Ignore small movements, so that a joystick resting slightly off centre reads as centred.
    const int16_t x_distance = get_distance(x);
    const int16_t y_distance = get_distance(y);
    if ((DEAD_ZONE > x_distance) && (DEAD_ZONE > y_distance)) { return JOYSTICK_CENTER; }

    // Report whichever axis has moved furthest, so that a diagonal picks the nearest direction.
    if (x_distance > y_distance) { return (0 < x) ? JOYSTICK_RIGHT : JOYSTICK_LEFT; }
    return (0 < y) ? JOYSTICK_UP : JOYSTICK_DOWN;
}

// -----------------------------------------------------------------------------
bool joystick_pressed(void)
{
    // The button grounds its pin, so a pressed button reads low.
    return 0U == (PORT.IN & (1U << BUTTON_PIN));
}
