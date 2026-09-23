/**
 * @file Application entry point.
 */
#include <stdint.h>

#include "driver/display.h"
#include "driver/joystick.h"
#include "driver/timer.h"

#define VALUE_MIN 0U  // Smallest value the display can show.
#define VALUE_MAX 99U // Largest value the display can show.
#define STEP_MS 200U  // Time the joystick has to be held for the value to change again.

/**
 * @brief Work out the value the joystick is asking for.
 *
 * @param[in] value Value shown at the moment.
 *
 * @return The value to show next.
 */
static uint8_t next_value(const uint8_t value)
{
    // Check if the joystick is pressed, reset the value if true.
    if (joystick_pressed()) { return VALUE_MIN; }

    // Push the joystick left or up to count up, and right or down to count down.
    const joystick_dir_t direction = joystick_read();

    switch (direction)
    {
        case JOYSTICK_LEFT:
        case JOYSTICK_UP:
            return VALUE_MAX == value ? VALUE_MIN : value + 1U;
        case JOYSTICK_RIGHT:
        case JOYSTICK_DOWN:
            return VALUE_MIN == value ? VALUE_MAX : value - 1U;
        default:
            return value;
    }
}

/**
 * @brief Run the application.
 *
 *        Counts on the display as the joystick is held: up or left counts up, down or right
 *        counts down, wrapping around at either end. Pressing the joystick starts over from
 *        zero.
 *
 * @return This function never returns.
 */
int main(void)
{
    joystick_init();
    display_init();
    const timer_id_t timer = timer_init(STEP_MS);
    timer_start(timer);
    uint8_t value = 0U;

    while (1)
    {
        if (timer_elapsed(timer))
        {
            value = next_value(value);
            display_write(value);
        }
        display_update();
    }
    return 0;
}
