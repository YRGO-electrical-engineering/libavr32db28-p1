/**
 * @file Tests for the AVR32DB28 switch driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/sw.h"
} // extern "C"

namespace
{
constexpr std::uint8_t zero{0U};

/** Bit masks of the pins the switches are connected to. SW1 is PC3, SW2 - SW3 are PF0 - PF1. */
constexpr std::uint8_t Sw1Mask{PIN3_bm};
constexpr std::uint8_t Sw2Mask{PIN0_bm};
constexpr std::uint8_t Sw3Mask{PIN1_bm};

/** Bit mask of the switches sharing I/O port F. */
constexpr std::uint8_t PortFSwitchMask{Sw2Mask | Sw3Mask};

/** Value of an input register in which every pin reads high. */
constexpr std::uint8_t AllPinsHigh{0xFFU};

/** Number of switches, and the number of values a switch ID can represent. */
constexpr std::uint8_t SwitchCount{3U};
constexpr std::uint8_t SwitchIdCount{4U};

/**
 * @brief Release every switch, i.e. drive all input pins low.
 *
 *        Each switch feeds +5 V to its pin when pressed, and an external resistor pulls the pin
 *        to ground when released, so a released switch reads low.
 */
void releaseAllSwitches()
{
    PORTC.IN = zero;
    PORTF.IN = zero;
}

/**
 * @brief Press a single switch by driving its pin high.
 *
 * @param[in] in Input register the switch is connected to.
 * @param[in] mask Bit mask of the switch pin.
 */
void pressSwitch(volatile std::uint8_t& in, const std::uint8_t mask)
{
    in = static_cast<std::uint8_t>(in | mask);
}

/**
 * @brief Test that initializing the switches configures every one of them as an input.
 *
 *        The direction bits are set beforehand, since the mock resets them to zero and a
 *        driver doing nothing at all would otherwise pass this test.
 */
TEST(Sw, InitConfiguresTheSwitchesAsInputs)
{
    constexpr std::uint8_t portCWithoutSw1{static_cast<std::uint8_t>(AllPinsHigh & ~Sw1Mask)};
    constexpr std::uint8_t portFWithoutSwitches{
        static_cast<std::uint8_t>(AllPinsHigh & ~PortFSwitchMask)};

    testHwPlatformReset();
    PORTC.DIR = AllPinsHigh;
    PORTF.DIR = AllPinsHigh;
    sw_init();

    EXPECT_EQ(PORTC.DIR, portCWithoutSw1);
    EXPECT_EQ(PORTF.DIR, portFWithoutSwitches);
}

/**
 * @brief Test that initializing the switches leaves the internal pull-ups off.
 *
 *        Every switch pin already has an external resistor to ground, and SW1 a capacitor for
 *        debouncing as well. An internal pull-up would fight that resistor and leave the pin
 *        somewhere between high and low.
 */
TEST(Sw, InitLeavesTheInternalPullUpsOff)
{
    testHwPlatformReset();
    PORTC.PIN3CTRL = PORT_PULLUPEN_bm;
    PORTF.PIN0CTRL = PORT_PULLUPEN_bm;
    PORTF.PIN1CTRL = PORT_PULLUPEN_bm;
    sw_init();

    EXPECT_EQ(PORTC.PIN3CTRL & PORT_PULLUPEN_bm, zero);
    EXPECT_EQ(PORTF.PIN0CTRL & PORT_PULLUPEN_bm, zero);
    EXPECT_EQ(PORTF.PIN1CTRL & PORT_PULLUPEN_bm, zero);
}

/**
 * @brief Test that initializing the switches leaves the neighbouring pins untouched.
 */
TEST(Sw, InitLeavesOtherPinsUntouched)
{
    testHwPlatformReset();
    sw_init();

    EXPECT_EQ(PORTC.PIN0CTRL, zero);
    EXPECT_EQ(PORTC.PIN1CTRL, zero);
    EXPECT_EQ(PORTC.PIN2CTRL, zero);
    EXPECT_EQ(PORTF.PIN2CTRL, zero);
    EXPECT_EQ(PORTA.DIR, zero);
    EXPECT_EQ(PORTD.DIR, zero);
}

/**
 * @brief Test that initializing the switches never drives their pins.
 */
TEST(Sw, InitDoesNotDriveTheSwitchPins)
{
    testHwPlatformReset();
    sw_init();

    EXPECT_EQ(PORTC.OUT, zero);
    EXPECT_EQ(PORTC.OUTSET, zero);
    EXPECT_EQ(PORTC.OUTCLR, zero);
    EXPECT_EQ(PORTF.OUT, zero);
    EXPECT_EQ(PORTF.OUTSET, zero);
    EXPECT_EQ(PORTF.OUTCLR, zero);
}

/**
 * @brief Test that a released switch reads as not pressed.
 */
TEST(Sw, ReadReturnsFalseWhenReleased)
{
    testHwPlatformReset();
    sw_init();
    releaseAllSwitches();

    EXPECT_FALSE(sw_read(SW1));
    EXPECT_FALSE(sw_read(SW2));
    EXPECT_FALSE(sw_read(SW3));
}

/**
 * @brief Test that a switch driving its pin high reads as pressed.
 */
TEST(Sw, ReadReturnsTrueWhenPressed)
{
    testHwPlatformReset();
    sw_init();

    releaseAllSwitches();
    pressSwitch(PORTC.IN, Sw1Mask);
    EXPECT_TRUE(sw_read(SW1));

    releaseAllSwitches();
    pressSwitch(PORTF.IN, Sw2Mask);
    EXPECT_TRUE(sw_read(SW2));

    releaseAllSwitches();
    pressSwitch(PORTF.IN, Sw3Mask);
    EXPECT_TRUE(sw_read(SW3));
}

/**
 * @brief Test that each switch reads its own pin, i.e. SW1 - SW3 map to PC3, PF0 and PF1.
 */
TEST(Sw, EachSwitchMapsToItsOwnPin)
{
    testHwPlatformReset();
    sw_init();

    releaseAllSwitches();
    pressSwitch(PORTC.IN, Sw1Mask);
    EXPECT_TRUE(sw_read(SW1));
    EXPECT_FALSE(sw_read(SW2));
    EXPECT_FALSE(sw_read(SW3));

    releaseAllSwitches();
    pressSwitch(PORTF.IN, Sw2Mask);
    EXPECT_FALSE(sw_read(SW1));
    EXPECT_TRUE(sw_read(SW2));
    EXPECT_FALSE(sw_read(SW3));

    releaseAllSwitches();
    pressSwitch(PORTF.IN, Sw3Mask);
    EXPECT_FALSE(sw_read(SW1));
    EXPECT_FALSE(sw_read(SW2));
    EXPECT_TRUE(sw_read(SW3));
}

/**
 * @brief Test that reading a switch is unaffected by the levels of the neighbouring pins.
 */
TEST(Sw, ReadIgnoresOtherPins)
{
    testHwPlatformReset();
    sw_init();

    // Every pin high except the switch pins, i.e. no switch is pressed.
    PORTC.IN = static_cast<std::uint8_t>(AllPinsHigh & ~Sw1Mask);
    PORTF.IN = static_cast<std::uint8_t>(AllPinsHigh & ~PortFSwitchMask);

    EXPECT_FALSE(sw_read(SW1));
    EXPECT_FALSE(sw_read(SW2));
    EXPECT_FALSE(sw_read(SW3));
}

/**
 * @brief Test that all three switches can be pressed at the same time.
 */
TEST(Sw, SwitchesCanBePressedTogether)
{
    testHwPlatformReset();
    sw_init();
    releaseAllSwitches();

    pressSwitch(PORTC.IN, Sw1Mask);
    pressSwitch(PORTF.IN, PortFSwitchMask);

    EXPECT_TRUE(sw_read(SW1));
    EXPECT_TRUE(sw_read(SW2));
    EXPECT_TRUE(sw_read(SW3));
}

/**
 * @brief Test that reading a switch doesn't disturb any register.
 */
TEST(Sw, ReadDoesNotWriteRegisters)
{
    testHwPlatformReset();
    (void)sw_read(SW1);
    (void)sw_read(SW2);
    (void)sw_read(SW3);

    EXPECT_EQ(PORTC.DIR, zero);
    EXPECT_EQ(PORTC.OUT, zero);
    EXPECT_EQ(PORTC.OUTSET, zero);
    EXPECT_EQ(PORTC.OUTCLR, zero);
    EXPECT_EQ(PORTC.PIN3CTRL, zero);
    EXPECT_EQ(PORTF.DIR, zero);
    EXPECT_EQ(PORTF.OUT, zero);
    EXPECT_EQ(PORTF.OUTSET, zero);
    EXPECT_EQ(PORTF.OUTCLR, zero);
    EXPECT_EQ(PORTF.PIN0CTRL, zero);
}

/**
 * @brief Test that only SW1 - SW3 are accepted, and that any other ID reads as not pressed.
 *
 *        Every pin is driven high, which is the state the driver reads as pressed, so an ID
 *        that slipped through the switch statement would show up as a spurious true here. The
 *        loop covers every value the enumeration can represent, which is one more than the
 *        three that are documented.
 */
TEST(Sw, EveryDocumentedSwitchIsValid)
{
    testHwPlatformReset();
    sw_init();
    PORTC.IN = AllPinsHigh;
    PORTF.IN = AllPinsHigh;
    std::uint8_t count{};

    for (std::uint8_t id{}; id < SwitchIdCount; ++id)
    {
        const auto sw         = static_cast<switch_id_t>(id);
        const auto documented = id <= SW3;

        EXPECT_EQ(sw_read(sw), documented);
        if (documented) { ++count; }
    }
    EXPECT_EQ(count, SwitchCount);
}

/**
 * @brief Test a full switch sequence: initialize, press and release.
 */
TEST(Sw, SwitchSequence)
{
    testHwPlatformReset();
    sw_init();
    releaseAllSwitches();
    EXPECT_FALSE(sw_read(SW2));

    pressSwitch(PORTF.IN, Sw2Mask);
    EXPECT_TRUE(sw_read(SW2));

    releaseAllSwitches();
    EXPECT_FALSE(sw_read(SW2));
}
} // namespace
