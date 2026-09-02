/**
 * @file AVR32DB28 7-segment display driver implementation details.
 */
#include <stdbool.h>
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/display.h"
#include "driver/timer.h"

#define PORT PORTA // Port register for the display.

#define BCD_MASK 0x0FU // Bit mask of the BCD pins (PORTA0 - PORTA3).
#define DP_PIN 4U      // Decimal point pin (PORTA4).
#define DIGIT1_PIN 5U  // Digit 1 select pin (PORTA5).
#define DIGIT2_PIN 6U  // Digit 2 select pin (PORTA6).

#define DIGIT_MASK ((1U << DIGIT1_PIN) | (1U << DIGIT2_PIN))  // Both digit selects.
#define DISPLAY_MASK (BCD_MASK | (1U << DP_PIN) | DIGIT_MASK) // Every pin the display uses.

#define BCD_BLANK 15U // BCD value blanking a digit; the decoder only knows 0 - 9.
#define VALUE_MAX 99U // Largest value two digits can show.
#define BASE 10U      // Number base, i.e. how the value is split between the digits.
#define NO_VALUE (-1) // Returned when the display is blank and has no value to report.

#define REFRESH_MS 5U // Time each digit is lit, giving the pair 100 refreshes per second.

/**
 * @brief Enumeration of digits.
 */
typedef enum
{
    DIGIT1, ///< The first digit, i.e. the tens.
    DIGIT2, ///< The second digit, i.e. the ones.
} digit_t;

/** Value being displayed (0 - 99). */
static uint8_t display_value = 0U;

/** True if the display is blank, false otherwise. */
static bool blanked = true;

/** Whether the decimal point is shown. */
static bool dp_shown = false;

/** Current digit. */
static digit_t current_digit = DIGIT1;

/** Timer deciding when to move on to the other digit. */
static timer_id_t refresh_timer = TIMER_ID_NONE;

// -----------------------------------------------------------------------------
static inline uint8_t get_tens(void) { return (uint8_t)(display_value / BASE); }

// -----------------------------------------------------------------------------
static inline uint8_t get_ones(void) { return (uint8_t)(display_value % BASE); }

// -----------------------------------------------------------------------------
static uint8_t get_bcd(const digit_t digit)
{
    // A blank display shows nothing on either digit.
    if (blanked) { return BCD_BLANK; }

    // The second digit always shows the ones.
    if (DIGIT2 == digit) { return get_ones(); }

    // The first digit shows the tens, blank when the value has none. The zero is kept when
    // the decimal point is shown, since 0.7 reads better than .7.
    const uint8_t tens = get_tens();
    return ((0U == tens) && !dp_shown) ? BCD_BLANK : tens;
}

// -----------------------------------------------------------------------------
static void show_digit(const digit_t digit)
{
    const uint8_t bcd = get_bcd(digit);

    // Split the pins into the ones to drive high and the ones to drive low, starting with the
    // BCD value the decoder is to translate into segments.
    uint8_t high = (uint8_t)(bcd & BCD_MASK);
    uint8_t low  = (uint8_t)(~bcd & BCD_MASK);

    // The digits share their segment lines, so only one of them may be lit at a time. A digit
    // lights when its own pin is driven low and the other one is left high.
    if (DIGIT1 == digit)
    {
        low |= (1U << DIGIT1_PIN);
        high |= (1U << DIGIT2_PIN);
    }
    else
    {
        low |= (1U << DIGIT2_PIN);
        high |= (1U << DIGIT1_PIN);
    }

    // The decimal point is a segment like any other, so it belongs to whichever digit happens
    // to be lit. Lighting it with the first digit places it between the two.
    if (dp_shown && !blanked && (DIGIT1 == digit)) { high |= (1U << DP_PIN); }
    else { low |= (1U << DP_PIN); }

    // Drive the pins high first and low second, so that the digit, which is selected by the
    // second write, lights only once its segments have settled.
    PORT.OUTSET = high;
    PORT.OUTCLR = low;
}

// -----------------------------------------------------------------------------
bool display_init(void)
{
    // Configure every pin the display uses as an output, leaving the relay on PORTA7 alone.
    PORT.DIR |= DISPLAY_MASK;

    // Blank the display: both digit selects high switches the digits off, and a BCD value above
    // nine leaves the decoder with nothing to show.
    PORT.OUTSET = DIGIT_MASK | BCD_MASK;
    PORT.OUTCLR = (1U << DP_PIN);

    // Start over from a blank display.
    display_value = 0U;
    blanked       = true;
    dp_shown      = false;
    current_digit = DIGIT1;

    // Reserve a timer to alternate the digits with.
    timer_deinit(refresh_timer);
    refresh_timer = timer_init(REFRESH_MS);
    timer_start(refresh_timer);

    // Return true if a timer was reserved, false otherwise.
    return TIMER_ID_NONE != refresh_timer;
}

// -----------------------------------------------------------------------------
void display_clear(void)
{
    // Blank the display, keeping the value so that it doesn't have to be written again.
    blanked = true;
}

// -----------------------------------------------------------------------------
int8_t display_read(void)
{
    // Return the current value if the display is not blank.
    return blanked ? NO_VALUE : (int8_t)(display_value);
}

// -----------------------------------------------------------------------------
void display_write(const uint8_t value)
{
    // Ignore a value two digits can't show.
    if (VALUE_MAX < value) { return; }
    display_value = value;
    blanked       = false;
}

// -----------------------------------------------------------------------------
void display_update(void)
{
    // Check if the refresh timer has elapsed, do nothing if not.
    if (!timer_elapsed(refresh_timer)) { return; }

    // Light the digit whose turn it is, then hand the next turn to the other one.
    show_digit(current_digit);
    current_digit = !current_digit;
}

// -----------------------------------------------------------------------------
void display_show_dp(const bool show)
{
    // Show or hide the decimal point from the next update onwards.
    dp_shown = show;
}
