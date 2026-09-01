/**
 * @file Tests for the AVR32DB28 relay driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/relay.h"
} // extern "C"

namespace
{
constexpr std::uint8_t zero{0U};

/** Bit mask of the pin driving the relay. The relay sits on PA7, see relay.c. */
constexpr std::uint8_t RelayMask{PIN7_bm};

/** Bit mask of the pins the 7-segment display uses, i.e. the rest of I/O port A. */
constexpr std::uint8_t DisplayMask{static_cast<std::uint8_t>(~RelayMask)};

/** Value of an output register in which every pin is driven high. */
constexpr std::uint8_t AllPinsHigh{0xFFU};

/**
 * @brief Test that initializing the relay configures its pin as an output.
 */
TEST(Relay, InitConfiguresTheRelayAsAnOutput)
{
    testHwPlatformReset();
    relay_init();

    EXPECT_EQ(PORTA.DIR, RelayMask);
}

/**
 * @brief Test that initializing the relay leaves the display pins as they were.
 *
 *        The relay shares I/O port A with the 7-segment display, so the direction bits have to
 *        be added to rather than assigned. Configuring the display first and then the relay is
 *        the order a program would use, and it catches an assignment wiping the display.
 */
TEST(Relay, InitLeavesTheDisplayPinsUntouched)
{
    testHwPlatformReset();
    PORTA.DIR = DisplayMask;
    relay_init();

    EXPECT_EQ(PORTA.DIR, AllPinsHigh);
}

/**
 * @brief Test that the relay is left open after initialization.
 *
 *        A relay closing by itself at start-up would switch whatever is wired to the screw
 *        terminal, so initialization must not drive the pin at all.
 */
TEST(Relay, InitLeavesTheRelayOpen)
{
    testHwPlatformReset();
    relay_init();

    EXPECT_EQ(PORTA.OUT, zero);
    EXPECT_EQ(PORTA.OUTSET, zero);
    EXPECT_EQ(PORTA.OUTCLR, zero);
    EXPECT_EQ(PORTA.OUTTGL, zero);
}

/**
 * @brief Test that initializing the relay leaves the other I/O ports untouched.
 */
TEST(Relay, InitLeavesOtherPortsUntouched)
{
    testHwPlatformReset();
    relay_init();

    EXPECT_EQ(PORTC.DIR, zero);
    EXPECT_EQ(PORTD.DIR, zero);
    EXPECT_EQ(PORTF.DIR, zero);
}

/**
 * @brief Test that switching the relay on sets its bit in OUTSET.
 */
TEST(Relay, WriteHighClosesTheRelay)
{
    testHwPlatformReset();
    relay_init();
    relay_write(true);

    EXPECT_EQ(PORTA.OUTSET, RelayMask);
    EXPECT_EQ(PORTA.OUTCLR, zero);
}

/**
 * @brief Test that switching the relay off sets its bit in OUTCLR.
 */
TEST(Relay, WriteLowOpensTheRelay)
{
    testHwPlatformReset();
    relay_init();
    relay_write(false);

    EXPECT_EQ(PORTA.OUTCLR, RelayMask);
    EXPECT_EQ(PORTA.OUTSET, zero);
}

/**
 * @brief Test that toggling the relay sets its bit in OUTTGL.
 *
 *        The hardware inverts the output itself, so the driver must not read the current state
 *        and write it back: OUTSET and OUTCLR have to stay untouched.
 */
TEST(Relay, ToggleInvertsTheRelay)
{
    testHwPlatformReset();
    relay_init();
    relay_toggle();

    EXPECT_EQ(PORTA.OUTTGL, RelayMask);
    EXPECT_EQ(PORTA.OUTSET, zero);
    EXPECT_EQ(PORTA.OUTCLR, zero);
}

/**
 * @brief Test that driving the relay leaves the other I/O ports untouched.
 */
TEST(Relay, WriteAndToggleLeaveOtherPortsUntouched)
{
    testHwPlatformReset();
    relay_init();
    relay_write(true);
    relay_toggle();

    EXPECT_EQ(PORTC.OUTSET, zero);
    EXPECT_EQ(PORTC.OUTTGL, zero);
    EXPECT_EQ(PORTD.OUTSET, zero);
    EXPECT_EQ(PORTD.OUTTGL, zero);
    EXPECT_EQ(PORTF.OUTSET, zero);
    EXPECT_EQ(PORTF.OUTTGL, zero);
}

/**
 * @brief Test that reading the relay returns the state held in the OUT register.
 *
 *        OUT is seeded directly rather than through relay_write, since the mock does not
 *        propagate OUTSET into OUT the way silicon does.
 */
TEST(Relay, ReadReturnsTheRelayState)
{
    testHwPlatformReset();
    relay_init();
    EXPECT_FALSE(relay_read());

    PORTA.OUT = RelayMask;
    EXPECT_TRUE(relay_read());
}

/**
 * @brief Test that reading the relay is unaffected by the levels of the display pins.
 */
TEST(Relay, ReadIgnoresTheOtherPins)
{
    testHwPlatformReset();
    relay_init();
    PORTA.OUT = DisplayMask;

    EXPECT_FALSE(relay_read());
}

/**
 * @brief Test that reading the relay doesn't disturb any register.
 */
TEST(Relay, ReadDoesNotWriteRegisters)
{
    testHwPlatformReset();
    (void)relay_read();

    EXPECT_EQ(PORTA.DIR, zero);
    EXPECT_EQ(PORTA.OUT, zero);
    EXPECT_EQ(PORTA.OUTSET, zero);
    EXPECT_EQ(PORTA.OUTCLR, zero);
    EXPECT_EQ(PORTA.OUTTGL, zero);
}

/**
 * @brief Test a full relay sequence: initialize, close, open and toggle.
 */
TEST(Relay, RelaySequence)
{
    testHwPlatformReset();
    relay_init();
    EXPECT_EQ(PORTA.DIR, RelayMask);

    relay_write(true);
    EXPECT_EQ(PORTA.OUTSET, RelayMask);

    relay_write(false);
    EXPECT_EQ(PORTA.OUTCLR, RelayMask);

    relay_toggle();
    EXPECT_EQ(PORTA.OUTTGL, RelayMask);
}
} // namespace
