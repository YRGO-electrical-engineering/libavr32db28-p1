/**
 * @file Tests for the AVR32DB28 joystick driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/joystick.h"
} // extern "C"

namespace
{
constexpr std::uint8_t zero{0U};

/** Bit mask of the pin the joystick's button is connected to, i.e. PD7. */
constexpr std::uint8_t ButtonMask{PIN7_bm};

/** Reading each axis gives at rest, and how far one has to move to count, see joystick.c. */
constexpr std::uint16_t center{2048U};
constexpr std::uint16_t deadZone{1024U};

/** Largest reading a 12-bit conversion produces, i.e. an axis pushed all the way over. */
constexpr std::uint16_t fullScale{4095U};

/** Value of an input register in which every pin reads high. */
constexpr std::uint8_t AllPinsHigh{0xFFU};

/**
 * @brief Hand the driver a reading for both axes.
 *
 *        The mocked ADC is plain storage with a single result register, so both axes read the
 *        same value within one call. Directions are therefore tested along the diagonal.
 *
 * @param[in] reading Value both axes are to report.
 */
void prepareAxes(const std::uint16_t reading)
{
    ADC0.RES      = reading;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

/**
 * @brief Release the joystick's button, i.e. let the pull-up hold its pin high.
 */
void releaseButton() { PORTD.IN = ButtonMask; }

/**
 * @brief Press the joystick's button, i.e. let it pull its pin to ground.
 */
void pressButton() { PORTD.IN = zero; }

/**
 * @brief Test that initializing the joystick is enough on its own.
 */
TEST(Joystick, InitPreparesTheAdc)
{
    testHwPlatformReset();
    joystick_init();

    EXPECT_EQ(ADC0.CTRLA, static_cast<std::uint8_t>(ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc));
    EXPECT_EQ(PORTD.PIN5CTRL, PORT_ISC_INPUT_DISABLE_gc);
    EXPECT_EQ(PORTD.PIN6CTRL, PORT_ISC_INPUT_DISABLE_gc);
}

/**
 * @brief Test that the button's pin is an input with its pull-up switched on.
 *
 *        The connector carries only the supply, ground and the three signals, so the module has
 *        to pull the pin down itself and nothing holds it high while the button is released.
 */
TEST(Joystick, InitPullsTheButtonPinUp)
{
    testHwPlatformReset();
    PORTD.DIR = AllPinsHigh;
    joystick_init();

    EXPECT_EQ(PORTD.DIR & ButtonMask, zero);
    EXPECT_NE(PORTD.PIN7CTRL & PORT_PULLUPEN_bm, zero);
}

/**
 * @brief Test that the button's pin keeps its digital input buffer.
 *
 *        The two axes are read as voltages and have their buffers switched off, but the button
 *        is read as a logic level and needs its own.
 */
TEST(Joystick, InitLeavesTheButtonsInputBufferOn)
{
    testHwPlatformReset();
    joystick_init();

    EXPECT_EQ(PORTD.PIN7CTRL & PORT_ISC_INPUT_DISABLE_gc, zero);
}

/**
 * @brief Test that a joystick at rest reports no direction.
 */
TEST(Joystick, ReadReturnsCenterAtRest)
{
    testHwPlatformReset();
    joystick_init();
    prepareAxes(center);

    EXPECT_EQ(joystick_read(), JOYSTICK_CENTER);
}

/**
 * @brief Test that a small movement is ignored.
 *
 *        A joystick rarely rests exactly in the middle, so without a dead zone it would report
 *        a direction constantly.
 */
TEST(Joystick, ReadIgnoresSmallMovements)
{
    testHwPlatformReset();
    joystick_init();

    prepareAxes(static_cast<std::uint16_t>(center + deadZone - 1U));
    EXPECT_EQ(joystick_read(), JOYSTICK_CENTER);

    prepareAxes(static_cast<std::uint16_t>(center - deadZone + 1U));
    EXPECT_EQ(joystick_read(), JOYSTICK_CENTER);
}

/**
 * @brief Test that a movement past the dead zone reports a direction.
 */
TEST(Joystick, ReadReportsADirectionPastTheDeadZone)
{
    testHwPlatformReset();
    joystick_init();

    prepareAxes(static_cast<std::uint16_t>(center + deadZone));
    EXPECT_NE(joystick_read(), JOYSTICK_CENTER);

    prepareAxes(static_cast<std::uint16_t>(center - deadZone));
    EXPECT_NE(joystick_read(), JOYSTICK_CENTER);
}

/**
 * @brief Test that a fully deflected joystick reports up or down.
 *
 *        Both axes read the same value here, so the joystick is on the diagonal and the driver
 *        picks the vertical axis, which is what it does when neither axis has moved further.
 */
TEST(Joystick, ReadReportsUpAndDown)
{
    testHwPlatformReset();
    joystick_init();

    prepareAxes(fullScale);
    EXPECT_EQ(joystick_read(), JOYSTICK_UP);

    prepareAxes(zero);
    EXPECT_EQ(joystick_read(), JOYSTICK_DOWN);
}

/**
 * @brief Test that a released button reads as not pressed.
 */
TEST(Joystick, PressedReturnsFalseWhenReleased)
{
    testHwPlatformReset();
    joystick_init();
    releaseButton();

    EXPECT_FALSE(joystick_pressed());
}

/**
 * @brief Test that a pressed button reads as pressed.
 *
 *        The button grounds its pin, so pressed reads low, the opposite of the pushbuttons on
 *        the board.
 */
TEST(Joystick, PressedReturnsTrueWhenPressed)
{
    testHwPlatformReset();
    joystick_init();
    pressButton();

    EXPECT_TRUE(joystick_pressed());
}

/**
 * @brief Test that the button is unaffected by the levels of the other pins.
 */
TEST(Joystick, PressedIgnoresTheOtherPins)
{
    testHwPlatformReset();
    joystick_init();

    // Every pin low except the button's, i.e. the button is released.
    PORTD.IN = ButtonMask;
    EXPECT_FALSE(joystick_pressed());

    // Every pin high except the button's, i.e. the button is pressed.
    PORTD.IN = static_cast<std::uint8_t>(AllPinsHigh & ~ButtonMask);
    EXPECT_TRUE(joystick_pressed());
}

/**
 * @brief Test that reading the joystick doesn't drive any I/O pin.
 */
TEST(Joystick, ReadDoesNotDriveAnyPin)
{
    testHwPlatformReset();
    joystick_init();
    prepareAxes(center);
    (void)joystick_read();
    (void)joystick_pressed();

    EXPECT_EQ(PORTD.OUT, zero);
    EXPECT_EQ(PORTD.OUTSET, zero);
    EXPECT_EQ(PORTD.OUTCLR, zero);
}
} // namespace
