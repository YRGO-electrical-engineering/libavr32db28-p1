/**
 * @file Tests for the AVR32DB28 LED driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/led.h"
} // extern "C"

namespace
{
constexpr std::uint8_t zero{0U};

/** Bit masks of the pins driving each LED. The LEDs sit on PC0 - PC2, see led.h. */
constexpr std::uint8_t RedLedMask{PIN0_bm};
constexpr std::uint8_t GreenLedMask{PIN1_bm};
constexpr std::uint8_t BlueLedMask{PIN2_bm};

/** Bit mask of all three LEDs. */
constexpr std::uint8_t AllLedsMask{RedLedMask | GreenLedMask | BlueLedMask};

/**
 * @brief Test that initializing the LEDs configures every one of them as an output.
 *
 *        All three direction bits must be set at once. Setting only one of them, e.g. by
 *        shifting a constant instead of the loop variable, is caught here.
 */
TEST(Led, InitConfiguresAllLedsAsOutputs)
{
    testHwPlatformReset();
    led_init();
    EXPECT_EQ(PORTC.DIR, AllLedsMask);
}

/**
 * @brief Test that the LEDs are left off after initialization.
 */
TEST(Led, InitLeavesTheLedsOff)
{
    testHwPlatformReset();
    led_init();

    EXPECT_EQ(PORTC.OUT, zero);
    EXPECT_EQ(PORTC.OUTSET, zero);
    EXPECT_EQ(PORTC.OUTCLR, zero);
    EXPECT_EQ(PORTC.OUTTGL, zero);
}

/**
 * @brief Test that initializing the LEDs leaves the other I/O ports untouched.
 */
TEST(Led, InitLeavesOtherPortsUntouched)
{
    testHwPlatformReset();
    led_init();

    EXPECT_EQ(PORTA.DIR, zero);
    EXPECT_EQ(PORTD.DIR, zero);
    EXPECT_EQ(PORTF.DIR, zero);
}

/**
 * @brief Test that turning a LED on sets the corresponding bit in OUTSET.
 */
TEST(Led, WriteHighTurnsLedOn)
{
    testHwPlatformReset();
    led_init();
    led_write(LED_RED, true);

    EXPECT_EQ(PORTC.OUTSET, RedLedMask);
    EXPECT_EQ(PORTC.OUTCLR, zero);
}

/**
 * @brief Test that turning a LED off sets the corresponding bit in OUTCLR.
 */
TEST(Led, WriteLowTurnsLedOff)
{
    testHwPlatformReset();
    led_init();
    led_write(LED_RED, false);

    EXPECT_EQ(PORTC.OUTCLR, RedLedMask);
    EXPECT_EQ(PORTC.OUTSET, zero);
}

/**
 * @brief Test that each LED drives its own pin, i.e. LED_RED - LED_BLUE map to PC0 - PC2.
 *
 *        The mock stores whatever the driver writes rather than merging it into OUT, so each
 *        write replaces the previous value of OUTSET instead of adding to it.
 */
TEST(Led, EachLedMapsToItsOwnPin)
{
    testHwPlatformReset();
    led_init();

    led_write(LED_RED, true);
    EXPECT_EQ(PORTC.OUTSET, RedLedMask);

    led_write(LED_GREEN, true);
    EXPECT_EQ(PORTC.OUTSET, GreenLedMask);

    led_write(LED_BLUE, true);
    EXPECT_EQ(PORTC.OUTSET, BlueLedMask);
}

/**
 * @brief Test that toggling a LED sets the corresponding bit in OUTTGL.
 *
 *        The hardware inverts the output itself, so the driver must not read the current state
 *        and write it back: OUTSET and OUTCLR have to stay untouched.
 */
TEST(Led, ToggleInvertsLed)
{
    testHwPlatformReset();
    led_init();
    led_toggle(LED_GREEN);

    EXPECT_EQ(PORTC.OUTTGL, GreenLedMask);
    EXPECT_EQ(PORTC.OUTSET, zero);
    EXPECT_EQ(PORTC.OUTCLR, zero);
}

/**
 * @brief Test that driving a LED leaves the other I/O ports untouched.
 */
TEST(Led, WriteAndToggleLeaveOtherPortsUntouched)
{
    testHwPlatformReset();
    led_init();
    led_write(LED_BLUE, true);
    led_toggle(LED_BLUE);

    EXPECT_EQ(PORTA.OUTSET, zero);
    EXPECT_EQ(PORTA.OUTCLR, zero);
    EXPECT_EQ(PORTA.OUTTGL, zero);
    EXPECT_EQ(PORTD.OUTSET, zero);
    EXPECT_EQ(PORTD.OUTCLR, zero);
    EXPECT_EQ(PORTD.OUTTGL, zero);
    EXPECT_EQ(PORTF.OUTSET, zero);
    EXPECT_EQ(PORTF.OUTCLR, zero);
    EXPECT_EQ(PORTF.OUTTGL, zero);
}

/**
 * @brief Test that reading a LED returns the state held in the OUT register.
 *
 *        OUT is seeded directly rather than through led_write, since the mock does not
 *        propagate OUTSET into OUT the way silicon does.
 */
TEST(Led, ReadReturnsLedState)
{
    testHwPlatformReset();
    led_init();
    PORTC.OUT = RedLedMask;

    EXPECT_TRUE(led_read(LED_RED));
    EXPECT_FALSE(led_read(LED_GREEN));
}

/**
 * @brief Test that reading a LED is unaffected by the states of the other LEDs.
 */
TEST(Led, ReadIgnoresOtherLeds)
{
    testHwPlatformReset();
    led_init();
    PORTC.OUT = static_cast<std::uint8_t>(~GreenLedMask);

    EXPECT_FALSE(led_read(LED_GREEN));
    EXPECT_TRUE(led_read(LED_RED));
    EXPECT_TRUE(led_read(LED_BLUE));
}

/**
 * @brief Test that reading a LED doesn't disturb any register.
 */
TEST(Led, ReadDoesNotWriteRegisters)
{
    testHwPlatformReset();
    (void)led_read(LED_RED);

    EXPECT_EQ(PORTC.DIR, zero);
    EXPECT_EQ(PORTC.OUT, zero);
    EXPECT_EQ(PORTC.OUTSET, zero);
    EXPECT_EQ(PORTC.OUTCLR, zero);
    EXPECT_EQ(PORTC.OUTTGL, zero);
}

/**
 * @brief Test that every LED can be driven, not just the ones used in the tests above.
 */
TEST(Led, EveryLedCanBeDriven)
{
    for (std::uint8_t id{LED_RED}; id <= LED_BLUE; ++id)
    {
        const auto led  = static_cast<led_id_t>(id);
        const auto mask = static_cast<std::uint8_t>(1U << id);

        testHwPlatformReset();
        led_init();

        led_write(led, true);
        EXPECT_EQ(PORTC.OUTSET, mask);

        led_write(led, false);
        EXPECT_EQ(PORTC.OUTCLR, mask);

        led_toggle(led);
        EXPECT_EQ(PORTC.OUTTGL, mask);
    }
}

/**
 * @brief Test a full LED sequence: initialize, turn on, turn off and toggle.
 */
TEST(Led, LedSequence)
{
    testHwPlatformReset();
    led_init();
    EXPECT_EQ(PORTC.DIR, AllLedsMask);

    led_write(LED_GREEN, true);
    EXPECT_EQ(PORTC.OUTSET, GreenLedMask);

    led_write(LED_GREEN, false);
    EXPECT_EQ(PORTC.OUTCLR, GreenLedMask);

    led_toggle(LED_GREEN);
    EXPECT_EQ(PORTC.OUTTGL, GreenLedMask);
}

} // namespace
