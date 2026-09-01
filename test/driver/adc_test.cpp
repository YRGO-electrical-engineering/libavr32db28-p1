/**
 * @file Tests for the AVR32DB28 ADC driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/adc.h"
} // extern "C"

namespace
{
constexpr std::uint8_t zero{0U};
constexpr std::uint16_t noValue{0U};

/** Largest value a 12-bit conversion can produce, and the supply voltage it stands for. */
constexpr std::uint16_t fullScale{4095U};
constexpr std::uint16_t supplyMv{5000U};

/** Sampling time mirroring SAMPLE_LENGTH in adc.c. */
constexpr std::uint8_t sampleLength{8U};

/** A channel ID the enumeration can represent but the driver doesn't know. */
constexpr auto unknownChannel{static_cast<adc_channel_t>(7U)};

/**
 * @brief A channel, together with the analog input it is expected to select.
 */
struct ChannelCase
{
    adc_channel_t channel; ///< Channel passed to adc_read.
    std::uint8_t muxpos;   ///< Analog input the channel is wired to.
};

/** Every channel the driver knows, and the analog input each one reads. */
constexpr ChannelCase Channels[]{{ADC_POT1, ADC_MUXPOS_AIN1_gc},
                                 {ADC_POT2, ADC_MUXPOS_AIN3_gc},
                                 {ADC_TEMP, ADC_MUXPOS_AIN4_gc},
                                 {ADC_JOYSTICK_Y, ADC_MUXPOS_AIN5_gc},
                                 {ADC_JOYSTICK_X, ADC_MUXPOS_AIN6_gc}};

/**
 * @brief Hand the driver a conversion result.
 *
 *        The mocked ADC is plain storage and never finishes a conversion by itself, so the
 *        result and the flag announcing it are placed there before the driver looks.
 *
 * @param[in] result Value the conversion is to produce.
 */
void prepareConversion(const std::uint16_t result)
{
    ADC0.RES      = result;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

/**
 * @brief Test that initializing the ADC measures against the supply voltage.
 */
TEST(Adc, InitMeasuresAgainstTheSupplyVoltage)
{
    testHwPlatformReset();
    adc_init();

    EXPECT_EQ(VREF.ADC0REF, VREF_REFSEL_VDD_gc);
}

/**
 * @brief Test that initializing the ADC enables it at the full resolution.
 */
TEST(Adc, InitEnablesTheAdcAtFullResolution)
{
    testHwPlatformReset();
    adc_init();

    EXPECT_EQ(ADC0.CTRLA, static_cast<std::uint8_t>(ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc));
}

/**
 * @brief Test that the ADC is clocked and sampled as the analog sources need.
 *
 *        The potentiometers are 10 kohm, which charges the sample capacitor slowly, so the
 *        sampling time is extended beyond the default.
 */
TEST(Adc, InitClocksAndSamplesTheAdc)
{
    testHwPlatformReset();
    adc_init();

    EXPECT_EQ(ADC0.CTRLC, ADC_PRESC_DIV4_gc);
    EXPECT_EQ(ADC0.SAMPCTRL, sampleLength);
}

/**
 * @brief Test that the analog pins have their digital input buffers switched off.
 *
 *        A pin sitting half way between high and low would otherwise leave its input buffer
 *        switching back and forth, which costs current and adds noise to the measurement.
 */
TEST(Adc, InitSwitchesOffTheDigitalInputBuffers)
{
    testHwPlatformReset();
    adc_init();

    EXPECT_EQ(PORTD.PIN1CTRL, PORT_ISC_INPUT_DISABLE_gc);
    EXPECT_EQ(PORTD.PIN3CTRL, PORT_ISC_INPUT_DISABLE_gc);
    EXPECT_EQ(PORTD.PIN4CTRL, PORT_ISC_INPUT_DISABLE_gc);
    EXPECT_EQ(PORTD.PIN5CTRL, PORT_ISC_INPUT_DISABLE_gc);
    EXPECT_EQ(PORTD.PIN6CTRL, PORT_ISC_INPUT_DISABLE_gc);
}

/**
 * @brief Test that the pins the ADC doesn't use are left alone.
 *
 *        PD7 is the joystick button, which is read as a logic level and needs its input buffer.
 */
TEST(Adc, InitLeavesTheDigitalPinsAlone)
{
    testHwPlatformReset();
    adc_init();

    EXPECT_EQ(PORTD.PIN0CTRL, zero);
    EXPECT_EQ(PORTD.PIN2CTRL, zero);
    EXPECT_EQ(PORTD.PIN7CTRL, zero);
    EXPECT_EQ(PORTD.DIR, zero);
}

/**
 * @brief Test that initializing the ADC leaves the other I/O ports untouched.
 */
TEST(Adc, InitLeavesOtherPortsUntouched)
{
    testHwPlatformReset();
    adc_init();

    EXPECT_EQ(PORTA.DIR, zero);
    EXPECT_EQ(PORTC.DIR, zero);
    EXPECT_EQ(PORTF.DIR, zero);
}

/**
 * @brief Test that each channel reads the analog input it is wired to.
 *
 *        Two channels selecting the same input, or a channel selecting the pin next to its own,
 *        would read a plausible but wrong voltage on the hardware.
 */
TEST(Adc, EachChannelReadsItsOwnAnalogInput)
{
    for (const auto& testCase : Channels)
    {
        testHwPlatformReset();
        adc_init();
        prepareConversion(fullScale);
        (void)adc_read(testCase.channel);

        EXPECT_EQ(ADC0.MUXPOS, testCase.muxpos);
    }
}

/**
 * @brief Test that reading a channel starts a conversion.
 */
TEST(Adc, ReadStartsAConversion)
{
    testHwPlatformReset();
    adc_init();
    prepareConversion(fullScale);
    (void)adc_read(ADC_POT1);

    EXPECT_EQ(ADC0.COMMAND, ADC_STCONV_bm);
}

/**
 * @brief Test that reading a channel returns the value the conversion produced.
 */
TEST(Adc, ReadReturnsTheConversionResult)
{
    constexpr std::uint16_t midScale{2048U};

    testHwPlatformReset();
    adc_init();

    prepareConversion(midScale);
    EXPECT_EQ(adc_read(ADC_POT1), midScale);

    prepareConversion(fullScale);
    EXPECT_EQ(adc_read(ADC_POT1), fullScale);

    prepareConversion(zero);
    EXPECT_EQ(adc_read(ADC_POT1), zero);
}

/**
 * @brief Test that reading a channel lowers the result flag, and only that flag.
 *
 *        The flag is cleared by writing a one to it on the real device, which the mocked
 *        register can't reproduce: it stores what is written, so a cleared flag and an
 *        untouched one look the same. Seeding a second flag makes the write observable, since
 *        the driver writes only the result flag and thereby wipes the other one.
 */
TEST(Adc, ReadLowersTheResultFlag)
{
    testHwPlatformReset();
    adc_init();
    ADC0.RES      = fullScale;
    ADC0.INTFLAGS = ADC_RESRDY_bm | ADC_WCMP_bm;
    (void)adc_read(ADC_POT1);

    EXPECT_EQ(ADC0.INTFLAGS, ADC_RESRDY_bm);
}

/**
 * @brief Test that an unknown channel reads as zero without starting a conversion.
 */
TEST(Adc, UnknownChannelReadsAsZero)
{
    testHwPlatformReset();
    adc_init();
    prepareConversion(fullScale);

    EXPECT_EQ(adc_read(unknownChannel), noValue);
    EXPECT_EQ(ADC0.COMMAND, zero);
}

/**
 * @brief Test that a conversion which never finishes is given up on.
 *
 *        The result flag is deliberately left down, which is what a misconfigured or disabled
 *        ADC looks like. The driver has to return rather than wait forever; this test finishing
 *        at all is the assertion.
 */
TEST(Adc, ReadGivesUpIfTheConversionNeverFinishes)
{
    testHwPlatformReset();
    adc_init();
    ADC0.RES = fullScale;

    EXPECT_EQ(adc_read(ADC_POT1), noValue);
}

/**
 * @brief Test that a reading is scaled to the voltage that produced it.
 *
 *        A full reading is one step short of the supply, since the conversion divides the range
 *        into 4096 steps and counts from zero.
 */
TEST(Adc, ReadMvScalesTheReadingToMillivolts)
{
    constexpr std::uint16_t midScale{2048U};
    constexpr std::uint16_t halfSupplyMv{2500U};
    constexpr std::uint16_t nearlySupplyMv{4998U};

    testHwPlatformReset();
    adc_init();

    prepareConversion(zero);
    EXPECT_EQ(adc_read_mv(ADC_POT1), zero);

    prepareConversion(midScale);
    EXPECT_EQ(adc_read_mv(ADC_POT1), halfSupplyMv);

    prepareConversion(fullScale);
    EXPECT_EQ(adc_read_mv(ADC_POT1), nearlySupplyMv);
}

/**
 * @brief Test that reading a voltage never exceeds the supply.
 */
TEST(Adc, ReadMvStaysWithinTheSupplyVoltage)
{
    testHwPlatformReset();
    adc_init();

    for (const auto& testCase : Channels)
    {
        prepareConversion(fullScale);
        EXPECT_TRUE(adc_read_mv(testCase.channel) < supplyMv);
    }
}

/**
 * @brief Test that reading an unknown channel reads as zero millivolts.
 */
TEST(Adc, ReadMvOfAnUnknownChannelIsZero)
{
    testHwPlatformReset();
    adc_init();
    prepareConversion(fullScale);

    EXPECT_EQ(adc_read_mv(unknownChannel), noValue);
}

/**
 * @brief Test that reading a channel doesn't drive any I/O pin.
 */
TEST(Adc, ReadDoesNotDriveAnyPin)
{
    testHwPlatformReset();
    adc_init();
    prepareConversion(fullScale);
    (void)adc_read(ADC_TEMP);

    EXPECT_EQ(PORTD.DIR, zero);
    EXPECT_EQ(PORTD.OUTSET, zero);
    EXPECT_EQ(PORTD.OUTCLR, zero);
    EXPECT_EQ(PORTA.OUTSET, zero);
    EXPECT_EQ(PORTC.OUTSET, zero);
}
} // namespace
