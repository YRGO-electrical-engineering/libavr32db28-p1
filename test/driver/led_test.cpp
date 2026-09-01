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

/** Counter top and clock divisor of the PWM timer, mirroring led.c. */
constexpr std::uint16_t PwmPeriod{99U};
constexpr std::uint8_t PwmPrescaler{TCA_SINGLE_CLKSEL_DIV16_gc};

/** Number of LEDs, i.e. of PWM channels. */
constexpr std::uint8_t ChannelCount{3U};

/**
 * @brief An LED, together with the timer registers driving it.
 */
struct PwmChannel
{
    led_id_t led;                    ///< LED the channel drives.
    std::uint8_t enable;             ///< Bit connecting the LED's pin to the timer.
    volatile std::uint16_t* compare; ///< Register holding the LED's duty cycle.
};

/**
 * @brief Get every LED together with the compare channel that dims it.
 *
 * @return The channels, in LED order.
 */
const PwmChannel* pwmChannels()
{
    static const PwmChannel channels[]{{LED_RED, TCA_SINGLE_CMP0EN_bm, &TCA0.SINGLE.CMP0},
                                       {LED_GREEN, TCA_SINGLE_CMP1EN_bm, &TCA0.SINGLE.CMP1},
                                       {LED_BLUE, TCA_SINGLE_CMP2EN_bm, &TCA0.SINGLE.CMP2}};
    return channels;
}

/**
 * @brief Check whether the timer is driving an LED's pin.
 *
 * @param[in] enable Bit connecting that LED's pin to the timer.
 *
 * @return True if the timer drives the pin, false if the output register does.
 */
bool timerDrivesPin(const std::uint8_t enable) { return (TCA0.SINGLE.CTRLB & enable) != zero; }

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

/**
 * @brief Test that initializing the LEDs routes the timer's outputs to their pins.
 *
 *        The timer drives port A by default, which is where the display and the relay live.
 */
TEST(Led, InitRoutesThePwmTimerToTheLeds)
{
    testHwPlatformReset();
    led_init();

    EXPECT_EQ(PORTMUX.TCAROUTEA, PORTMUX_TCA0_PORTC_gc);
}

/**
 * @brief Test that the PWM timer runs, and counts to a top that makes a compare a percentage.
 */
TEST(Led, InitStartsThePwmTimer)
{
    testHwPlatformReset();
    led_init();

    EXPECT_EQ(TCA0.SINGLE.PER, PwmPeriod);
    EXPECT_EQ(TCA0.SINGLE.CTRLA, static_cast<std::uint8_t>(PwmPrescaler | TCA_SINGLE_ENABLE_bm));
    EXPECT_EQ(TCA0.SINGLE.CTRLB & TCA_SINGLE_WGMODE_SINGLESLOPE_gc,
              TCA_SINGLE_WGMODE_SINGLESLOPE_gc);
}

/**
 * @brief Test that the LEDs are switched, not dimmed, until PWM is asked for.
 */
TEST(Led, InitLeavesThePinsWithTheOutputRegister)
{
    testHwPlatformReset();
    led_init();

    for (std::uint8_t i{zero}; i < ChannelCount; ++i)
    {
        EXPECT_FALSE(timerDrivesPin(pwmChannels()[i].enable));
    }
}

/**
 * @brief Test that a duty cycle is handed to the timer as it stands.
 *
 *        The counter tops out at 99, so the compare value is the percentage itself and nobody
 *        reading the driver has to follow any arithmetic.
 */
TEST(Led, PwmSetsTheDutyCycleAsAPercentage)
{
    constexpr std::uint8_t halfBrightness{50U};

    testHwPlatformReset();
    led_init();
    led_pwm(LED_RED, halfBrightness);

    EXPECT_EQ(TCA0.SINGLE.CMP0, halfBrightness);
}

/**
 * @brief Test that dimming an LED hands its pin over to the timer.
 */
TEST(Led, PwmGivesThePinToTheTimer)
{
    constexpr std::uint8_t brightness{30U};

    testHwPlatformReset();
    led_init();
    led_pwm(LED_GREEN, brightness);

    EXPECT_TRUE(timerDrivesPin(TCA_SINGLE_CMP1EN_bm));
}

/**
 * @brief Test that each LED is dimmed by its own compare channel.
 *
 *        Two LEDs sharing a channel would dim together, and the RGB LED could never mix a
 *        colour.
 */
TEST(Led, EachLedHasItsOwnPwmChannel)
{
    constexpr std::uint8_t red{20U};
    constexpr std::uint8_t green{50U};
    constexpr std::uint8_t blue{80U};

    testHwPlatformReset();
    led_init();
    led_pwm(LED_RED, red);
    led_pwm(LED_GREEN, green);
    led_pwm(LED_BLUE, blue);

    EXPECT_EQ(TCA0.SINGLE.CMP0, red);
    EXPECT_EQ(TCA0.SINGLE.CMP1, green);
    EXPECT_EQ(TCA0.SINGLE.CMP2, blue);
}

/**
 * @brief Test that all three LEDs can be dimmed at the same time.
 *
 *        This is what the RGB LED needs in order to show a mixed colour, and what a driver
 *        dimming one LED at a time can't do.
 */
TEST(Led, EveryLedCanBeDimmedAtOnce)
{
    constexpr std::uint8_t brightness{50U};

    testHwPlatformReset();
    led_init();

    for (std::uint8_t i{zero}; i < ChannelCount; ++i)
    {
        led_pwm(pwmChannels()[i].led, brightness);
    }

    for (std::uint8_t i{zero}; i < ChannelCount; ++i)
    {
        EXPECT_TRUE(timerDrivesPin(pwmChannels()[i].enable));
        EXPECT_EQ(*pwmChannels()[i].compare, brightness);
    }
}

/**
 * @brief Test that a duty cycle above 100 percent is ignored.
 */
TEST(Led, PwmIgnoresDutyCycleAboveHundred)
{
    constexpr std::uint8_t halfBrightness{50U};
    constexpr std::uint8_t justAboveMax{101U};
    constexpr std::uint8_t largest{255U};

    testHwPlatformReset();
    led_init();
    led_pwm(LED_RED, halfBrightness);
    led_pwm(LED_RED, justAboveMax);
    led_pwm(LED_RED, largest);

    EXPECT_EQ(TCA0.SINGLE.CMP0, halfBrightness);
}

/**
 * @brief Test that switching an LED takes its pin back from the timer.
 *
 *        The timer drives the pin while an LED is dimmed, so without this led_write would be
 *        writing to an output register that nothing is listening to.
 */
TEST(Led, WriteTakesThePinBackFromTheTimer)
{
    constexpr std::uint8_t brightness{50U};

    testHwPlatformReset();
    led_init();
    led_pwm(LED_RED, brightness);
    led_write(LED_RED, true);

    EXPECT_FALSE(timerDrivesPin(TCA_SINGLE_CMP0EN_bm));
    EXPECT_EQ(PORTC.OUTSET, RedLedMask);
}

/**
 * @brief Test that toggling an LED takes its pin back from the timer as well.
 */
TEST(Led, ToggleTakesThePinBackFromTheTimer)
{
    constexpr std::uint8_t brightness{50U};

    testHwPlatformReset();
    led_init();
    led_pwm(LED_BLUE, brightness);
    led_toggle(LED_BLUE);

    EXPECT_FALSE(timerDrivesPin(TCA_SINGLE_CMP2EN_bm));
    EXPECT_EQ(PORTC.OUTTGL, BlueLedMask);
}

/**
 * @brief Test that switching one LED leaves a dimmed one dimmed.
 */
TEST(Led, WriteLeavesTheOtherLedsDimmed)
{
    constexpr std::uint8_t brightness{40U};

    testHwPlatformReset();
    led_init();
    led_pwm(LED_GREEN, brightness);
    led_write(LED_RED, true);

    EXPECT_TRUE(timerDrivesPin(TCA_SINGLE_CMP1EN_bm));
    EXPECT_EQ(TCA0.SINGLE.CMP1, brightness);
}

/**
 * @brief Test that a dimmed LED reads as lit, and a fully dimmed one as dark.
 */
TEST(Led, ReadReportsWhetherADimmedLedIsLit)
{
    constexpr std::uint8_t dimmest{1U};

    testHwPlatformReset();
    led_init();

    led_pwm(LED_RED, dimmest);
    EXPECT_TRUE(led_read(LED_RED));

    led_pwm(LED_RED, zero);
    EXPECT_FALSE(led_read(LED_RED));
}

} // namespace
