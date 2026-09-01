/**
 * @brief Application entry point.
 */
#include <stdint.h>

#include "driver/display.h"
#include "driver/joystick.h"
#include "driver/timer.h"

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
    // Push the joystick up to count up and down to count down, stopping at either end.
    const joystick_dir_t direction = joystick_read();

    if ((JOYSTICK_UP == direction) && (VALUE_MAX > value)) { return (uint8_t)(value + 1U); }
    if ((JOYSTICK_DOWN == direction) && (0U < value)) { return (uint8_t)(value - 1U); }
    return value;
}

/**
 * @brief Run the application.
 *
 *        Counts up and down on the display as the joystick is held up or down, and starts over
 *        from zero when the joystick is pressed.
 *
 * @return This function never returns.
 */
int main(void)
{
    // Initialize the display and the joystick.
    display_init();
    joystick_init();

    // Reserve a timer, so that the value changes a few times a second rather than every lap.
    const timer_id_t step_timer = timer_init(STEP_MS);
    timer_start(step_timer);
    uint8_t value = 0U;

    while (1)
    {
        // Press the joystick to start over from zero.
        if (joystick_pressed()) { value = 0U; }

        // Otherwise count up or down, as long as the joystick is held to one side.
        else if (timer_elapsed(step_timer)) { value = next_value(value); }

        // Show the value. Without display_update every lap the digits never light up.
        display_write(value);
        display_update();
    }
    return 0;
}
