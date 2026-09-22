/**
 * @brief Application entry point.
 */
#include <stdint.h>

#include "driver/display.h"
#include "driver/timer.h"

#define VALUE_MAX 99U               // Largest value the display can show.
#define VALUE_WRAP (VALUE_MAX + 1U) // Value the count wraps at, starting over from zero.
#define STEP_MS 200U                // Time between each step of the count.

/**
 * @brief Run the application.
 *
 *        Counts up on the display, one step every STEP_MS milliseconds, and starts over from
 *        zero after VALUE_MAX.
 *
 * @return This function never returns.
 */
int main(void)
{
    // Initialize the display.
    display_init();

    // Reserve a timer, so that the value changes a few times a second rather than every lap.
    const timer_id_t step_timer = timer_init(STEP_MS);
    timer_start(step_timer);
    uint8_t value = 0U;

    while (1)
    {
        // Count up each time the timer elapses, wrapping from VALUE_MAX back to zero.
        if (timer_elapsed(step_timer))
        {
            value++;
            value = value % (VALUE_WRAP);
        }

        // Show the value. Without display_update every lap the digits never light up.
        display_write(value);
        display_update();
    }
    return 0;
}
