/**
 * @file Tests for the AVR32DB28 hex display driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/display.h"
#include "driver/timer.h"
} // extern "C"

namespace
{
constexpr std::uint8_t zero{0U};

/** Bit masks of the pins the display uses. It sits on PA0 - PA6, see display.c. */
constexpr std::uint8_t BcdMask{0x0FU};
constexpr std::uint8_t DpMask{PIN4_bm};
constexpr std::uint8_t Digit1Mask{PIN5_bm};
constexpr std::uint8_t Digit2Mask{PIN6_bm};
constexpr std::uint8_t DisplayMask{
    static_cast<std::uint8_t>(BcdMask | DpMask | Digit1Mask | Digit2Mask)};

/** Bit mask of the relay, which shares I/O port A with the display. */
constexpr std::uint8_t RelayMask{PIN7_bm};

/** BCD value the decoder blanks a digit for, mirroring BCD_BLANK in display.c. */
constexpr std::uint8_t BcdBlank{15U};

/** Time each digit is lit in milliseconds, mirroring REFRESH_MS in display.c. */
constexpr std::uint16_t RefreshMs{5U};

/** Counter top value producing a one millisecond tick at 4 MHz with a /2 prescaler. */
constexpr std::uint16_t TimerPeriod{1999U};

/** Number of timer circuits the device provides. */
constexpr std::uint8_t TimerCount{static_cast<std::uint8_t>(TIMER_ID_NONE)};

/** Two digit value most tests write, along with the two digits it is shown as. */
constexpr std::uint8_t displayValue{42U};
constexpr std::uint8_t displayTens{displayValue / 10U};
constexpr std::uint8_t displayOnes{displayValue % 10U};

/** Single digit value, i.e. one shown without a leading zero. */
constexpr std::uint8_t singleDigitValue{7U};

/** Two digit value written where only the decimal point is of interest. */
constexpr std::uint8_t decimalPointValue{37U};

/** Number of refresh periods stepped through where several in a row are checked. */
constexpr std::uint8_t refreshCycles{4U};

/**
 * @brief Release every timer, reset the mocked registers and initialize the display.
 *
 *        The display reserves a timer of its own, so the timers are released first: that hands
 *        the display TCB0 in every test regardless of what ran before it.
 */
void reset()
{
    for (std::uint8_t id{zero}; id < TimerCount; ++id)
    {
        timer_deinit(static_cast<timer_id_t>(id));
    }
    testHwPlatformReset();
    display_init();
}

/**
 * @brief Let one digit's time run out, so that the display moves on to the next digit.
 *
 *        The circuit's flag is raised as the hardware does once per millisecond and lowered
 *        again afterwards, since the mocked register keeps whatever is written to it rather
 *        than clearing on a write of one.
 */
void elapseRefresh()
{
    for (std::uint16_t i{zero}; i < RefreshMs; ++i)
    {
        TCB0.INTFLAGS = TCB_CAPT_bm;
        display_update();
        TCB0.INTFLAGS = zero;
    }
}

/**
 * @brief Get the BCD value the decoder is being fed.
 *
 * @return Value on the four BCD pins.
 */
std::uint8_t shownBcd() { return static_cast<std::uint8_t>(PORTA.OUTSET & BcdMask); }

/**
 * @brief Check whether a digit is lit.
 *
 *        A digit lights when its own pin is driven low, so both registers have to agree: the
 *        pin appears in OUTCLR and not in OUTSET.
 *
 * @param[in] mask Bit mask of the digit's pin.
 *
 * @return True if the digit is lit, false otherwise.
 */
bool digitLit(const std::uint8_t mask)
{
    return ((PORTA.OUTCLR & mask) != zero) && ((PORTA.OUTSET & mask) == zero);
}

/**
 * @brief Check whether the decimal point is lit.
 *
 * @return True if the decimal point is lit, false otherwise.
 */
bool dpLit() { return ((PORTA.OUTSET & DpMask) != zero) && ((PORTA.OUTCLR & DpMask) == zero); }

/**
 * @brief Test that initializing the display configures every pin it uses as an output.
 */
TEST(Display, InitConfiguresTheDisplayPinsAsOutputs)
{
    reset();
    EXPECT_EQ(PORTA.DIR, DisplayMask);
}

/**
 * @brief Test that initializing the display leaves the relay pin as it was.
 *
 *        The display shares I/O port A with the relay, so the direction bits have to be added
 *        to rather than assigned. A relay switched back to an input would release whatever is
 *        wired to the screw terminal.
 */
TEST(Display, InitLeavesTheRelayPinUntouched)
{
    for (std::uint8_t id{zero}; id < TimerCount; ++id)
    {
        timer_deinit(static_cast<timer_id_t>(id));
    }
    testHwPlatformReset();
    PORTA.DIR = RelayMask;
    display_init();

    EXPECT_EQ(PORTA.DIR, static_cast<std::uint8_t>(DisplayMask | RelayMask));
}

/**
 * @brief Test that the display starts out blank.
 *
 *        Both digit selects are driven high, which switches the digits off, and the decoder is
 *        given a value above nine, which leaves it with nothing to show.
 */
TEST(Display, InitBlanksTheDisplay)
{
    reset();

    EXPECT_EQ(PORTA.OUTSET, static_cast<std::uint8_t>(BcdMask | Digit1Mask | Digit2Mask));
    EXPECT_EQ(PORTA.OUTCLR, DpMask);
    EXPECT_FALSE(digitLit(Digit1Mask));
    EXPECT_FALSE(digitLit(Digit2Mask));
}

/**
 * @brief Test that initializing the display reserves a running timer to refresh itself with.
 */
TEST(Display, InitReservesAndStartsARefreshTimer)
{
    reset();

    EXPECT_EQ(TCB0.CCMP, TimerPeriod);
    EXPECT_NE(TCB0.CTRLA & TCB_ENABLE_bm, zero);
}

/**
 * @brief Test that initializing the display twice doesn't reserve a second timer.
 *
 *        Three timers exist in total, so a display quietly taking one more on every call would
 *        leave a program without any.
 */
TEST(Display, InitTwiceReservesOneTimerOnly)
{
    reset();
    display_init();

    EXPECT_NE(TCB0.CTRLA & TCB_ENABLE_bm, zero);
    EXPECT_EQ(TCB1.CTRLA, zero);
    EXPECT_EQ(TCB2.CTRLA, zero);
}

/**
 * @brief Test that a digit stays lit until its time is up.
 *
 *        The digits alternate on a timer rather than on every call, so that the program can
 *        call display_update() as often as it likes.
 */
TEST(Display, UpdateWaitsForTheRefreshTime)
{
    reset();
    display_write(displayValue);

    // One millisecond short of a digit's time, the display hasn't changed since initialization.
    for (std::uint16_t i{zero}; i < RefreshMs - 1U; ++i)
    {
        TCB0.INTFLAGS = TCB_CAPT_bm;
        display_update();
        TCB0.INTFLAGS = zero;
    }
    EXPECT_EQ(PORTA.OUTSET, static_cast<std::uint8_t>(BcdMask | Digit1Mask | Digit2Mask));

    // The final millisecond lights the first digit.
    TCB0.INTFLAGS = TCB_CAPT_bm;
    display_update();
    EXPECT_TRUE(digitLit(Digit1Mask));
}

/**
 * @brief Test that the two digits are lit alternately, tens first.
 */
TEST(Display, UpdateAlternatesBetweenTheDigits)
{
    reset();
    display_write(displayValue);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), displayTens);
    EXPECT_TRUE(digitLit(Digit1Mask));

    elapseRefresh();
    EXPECT_EQ(shownBcd(), displayOnes);
    EXPECT_TRUE(digitLit(Digit2Mask));

    elapseRefresh();
    EXPECT_EQ(shownBcd(), displayTens);
    EXPECT_TRUE(digitLit(Digit1Mask));
}

/**
 * @brief Test that only one digit is ever lit.
 *
 *        Both digits share the same segment lines, so lighting them together would show the
 *        same value twice rather than two different ones.
 */
TEST(Display, OnlyOneDigitIsLitAtATime)
{
    reset();
    display_write(displayValue);

    for (std::uint8_t i{zero}; i < refreshCycles; ++i)
    {
        elapseRefresh();
        EXPECT_NE(digitLit(Digit1Mask), digitLit(Digit2Mask));
    }
}

/**
 * @brief Test that the BCD pins carry the complement of the value as well.
 *
 *        The decoder reads all four pins, so the driver has to drive the zeroes low as well as
 *        the ones high; leaving the rest alone would show whatever the previous digit set.
 */
TEST(Display, UpdateDrivesEveryBcdPin)
{
    reset();
    display_write(displayValue);
    elapseRefresh();

    EXPECT_EQ(PORTA.OUTSET & BcdMask, displayTens);
    EXPECT_EQ(PORTA.OUTCLR & BcdMask, static_cast<std::uint8_t>(~displayTens & BcdMask));
}

/**
 * @brief Test that a value below ten is shown without a leading zero.
 */
TEST(Display, LeadingZeroIsBlanked)
{
    reset();
    display_write(singleDigitValue);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), singleDigitValue);
}

/**
 * @brief Test that the leading zero is kept when the decimal point is shown.
 *
 *        Blanking it would show .7 rather than 0.7.
 */
TEST(Display, LeadingZeroIsKeptWithTheDecimalPoint)
{
    reset();
    display_write(singleDigitValue);
    display_show_dp(true);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), zero);
    EXPECT_TRUE(dpLit());
}

/**
 * @brief Test that the decimal point is lit with the first digit only.
 *
 *        One pin drives the decimal point of both digits, so it appears wherever it is lit.
 *        Lighting it with the second digit as well would put a dot after both digits.
 */
TEST(Display, DecimalPointBelongsToTheFirstDigit)
{
    reset();
    display_write(decimalPointValue);
    display_show_dp(true);

    elapseRefresh();
    EXPECT_TRUE(digitLit(Digit1Mask));
    EXPECT_TRUE(dpLit());

    elapseRefresh();
    EXPECT_TRUE(digitLit(Digit2Mask));
    EXPECT_FALSE(dpLit());
}

/**
 * @brief Test that the decimal point stays dark until it is asked for, and goes dark again.
 */
TEST(Display, DecimalPointIsHiddenByDefault)
{
    reset();
    display_write(decimalPointValue);

    elapseRefresh();
    EXPECT_FALSE(dpLit());

    display_show_dp(true);
    elapseRefresh();
    elapseRefresh();
    EXPECT_TRUE(dpLit());

    display_show_dp(false);
    elapseRefresh();
    elapseRefresh();
    EXPECT_FALSE(dpLit());
}

/**
 * @brief Test that a value too large for two digits is ignored.
 */
TEST(Display, WriteIgnoresValuesAboveNinetyNine)
{
    constexpr std::uint8_t justAboveMax{100U};
    constexpr std::uint8_t largest{255U};

    reset();
    display_write(displayValue);
    display_write(justAboveMax);
    display_write(largest);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), displayTens);
}

/**
 * @brief Test that clearing the display blanks both digits.
 */
TEST(Display, ClearBlanksBothDigits)
{
    reset();
    display_write(displayValue);
    display_show_dp(true);
    display_clear();

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);
    EXPECT_FALSE(dpLit());

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);
    EXPECT_FALSE(dpLit());
}

/**
 * @brief Test that a cleared display shows a value written afterwards.
 */
TEST(Display, WriteAfterClearShowsTheValueAgain)
{
    reset();
    display_clear();
    display_write(displayValue);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), displayTens);
}

/**
 * @brief Test that refreshing the display never drives the relay pin.
 */
TEST(Display, UpdateLeavesTheRelayPinUntouched)
{
    reset();
    display_write(displayValue);

    for (std::uint8_t i{zero}; i < refreshCycles; ++i)
    {
        elapseRefresh();
        EXPECT_EQ(PORTA.OUTSET & RelayMask, zero);
        EXPECT_EQ(PORTA.OUTCLR & RelayMask, zero);
    }
}

/**
 * @brief Test that the display leaves the other I/O ports untouched.
 */
TEST(Display, DisplayLeavesOtherPortsUntouched)
{
    reset();
    display_write(displayValue);
    elapseRefresh();
    elapseRefresh();

    EXPECT_EQ(PORTC.DIR, zero);
    EXPECT_EQ(PORTC.OUTSET, zero);
    EXPECT_EQ(PORTD.DIR, zero);
    EXPECT_EQ(PORTD.OUTSET, zero);
    EXPECT_EQ(PORTF.DIR, zero);
    EXPECT_EQ(PORTF.OUTSET, zero);
}
} // namespace
