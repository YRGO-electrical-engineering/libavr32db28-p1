/**
 * @file Tests for the AVR32DB28 potentiometer driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/pot.h"
} // extern "C"

namespace
{
constexpr std::uint8_t zero{0U};

/** Largest value a 12-bit conversion can produce. */
constexpr std::uint16_t fullScale{4095U};

/**
 * @brief Hand the driver a conversion result.
 *
 *        The mocked ADC never finishes a conversion by itself, so the result and the flag
 *        announcing it are placed there before the driver looks.
 *
 * @param[in] result Value the conversion is to produce.
 */
void prepareConversion(const std::uint16_t result)
{
    ADC0.RES      = result;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

/**
 * @brief Test that initializing the potentiometers is enough on its own.
 *
 *        A program reading a potentiometer shouldn't have to know that an ADC is involved, so
 *        pot_init has to leave the converter ready to use.
 */
TEST(Pot, InitPreparesTheAdc)
{
    testHwPlatformReset();
    pot_init();

    EXPECT_EQ(ADC0.CTRLA, static_cast<std::uint8_t>(ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc));
    EXPECT_EQ(VREF.ADC0REF, VREF_REFSEL_VDD_gc);
}

/**
 * @brief Test that initializing the potentiometers prepares both of their pins.
 */
TEST(Pot, InitPreparesBothPins)
{
    testHwPlatformReset();
    pot_init();

    EXPECT_EQ(PORTD.PIN1CTRL, PORT_ISC_INPUT_DISABLE_gc);
    EXPECT_EQ(PORTD.PIN3CTRL, PORT_ISC_INPUT_DISABLE_gc);
}

/**
 * @brief Test that each potentiometer reads the analog input it is wired to.
 *
 *        The second potentiometer is on PD3, i.e. AIN3, and not on the PD2 next to it, which is
 *        wired to nothing.
 */
TEST(Pot, EachPotReadsItsOwnAnalogInput)
{
    testHwPlatformReset();
    pot_init();

    prepareConversion(fullScale);
    (void)pot_read(POT1);
    EXPECT_EQ(ADC0.MUXPOS, ADC_MUXPOS_AIN1_gc);

    prepareConversion(fullScale);
    (void)pot_read(POT2);
    EXPECT_EQ(ADC0.MUXPOS, ADC_MUXPOS_AIN3_gc);
}

/**
 * @brief Test that reading a potentiometer starts a conversion and returns its result.
 */
TEST(Pot, ReadReturnsTheConversionResult)
{
    constexpr std::uint16_t midScale{2048U};

    testHwPlatformReset();
    pot_init();

    prepareConversion(midScale);
    EXPECT_EQ(pot_read(POT1), midScale);
    EXPECT_EQ(ADC0.COMMAND, ADC_STCONV_bm);

    prepareConversion(zero);
    EXPECT_EQ(pot_read(POT2), zero);
}

/**
 * @brief Test that a potentiometer's voltage is reported in millivolts.
 *
 *        A full reading is one step short of the supply, since the conversion divides the range
 *        into 4096 steps and counts from zero.
 */
TEST(Pot, ReadMvScalesTheReadingToMillivolts)
{
    constexpr std::uint16_t midScale{2048U};
    constexpr std::uint16_t halfSupplyMv{2500U};
    constexpr std::uint16_t nearlySupplyMv{4998U};

    testHwPlatformReset();
    pot_init();

    prepareConversion(midScale);
    EXPECT_EQ(pot_read_mv(POT1), halfSupplyMv);

    prepareConversion(fullScale);
    EXPECT_EQ(pot_read_mv(POT2), nearlySupplyMv);

    prepareConversion(zero);
    EXPECT_EQ(pot_read_mv(POT1), zero);
}

/**
 * @brief Test that each potentiometer reads its own input in millivolts as well.
 */
TEST(Pot, ReadMvUsesTheRightAnalogInput)
{
    testHwPlatformReset();
    pot_init();

    prepareConversion(fullScale);
    (void)pot_read_mv(POT2);
    EXPECT_EQ(ADC0.MUXPOS, ADC_MUXPOS_AIN3_gc);

    prepareConversion(fullScale);
    (void)pot_read_mv(POT1);
    EXPECT_EQ(ADC0.MUXPOS, ADC_MUXPOS_AIN1_gc);
}

/**
 * @brief Test that a conversion which never finishes reads as zero rather than hanging.
 */
TEST(Pot, ReadGivesUpIfTheConversionNeverFinishes)
{
    testHwPlatformReset();
    pot_init();
    ADC0.RES = fullScale;

    EXPECT_EQ(pot_read(POT1), zero);
    EXPECT_EQ(pot_read_mv(POT2), zero);
}

/**
 * @brief Test that reading a potentiometer doesn't drive any I/O pin.
 */
TEST(Pot, ReadDoesNotDriveAnyPin)
{
    testHwPlatformReset();
    pot_init();
    prepareConversion(fullScale);
    (void)pot_read(POT1);

    EXPECT_EQ(PORTD.DIR, zero);
    EXPECT_EQ(PORTD.OUTSET, zero);
    EXPECT_EQ(PORTD.OUTCLR, zero);
}
} // namespace
