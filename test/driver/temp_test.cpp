/**
 * @file Tests for the AVR32DB28 temperature sensor driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/temp.h"
} // extern "C"

namespace
{
constexpr std::uint8_t zero{0U};

/** Steps a 12-bit conversion is divided into, and the supply voltage they cover. */
constexpr std::uint32_t steps{4096U};
constexpr std::uint32_t supplyMv{5000U};

/**
 * @brief Hand the driver a sensor voltage.
 *
 *        The mocked ADC never converts anything by itself, so the conversion the given voltage
 *        would have produced is worked out and placed in the result register.
 *
 * @param[in] mv Voltage the sensor is to be reporting, in millivolts.
 */
void prepareVoltage(const std::uint16_t mv)
{
    ADC0.RES      = static_cast<std::uint16_t>((mv * steps + supplyMv / 2U) / supplyMv);
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

/**
 * @brief Test that initializing the sensor is enough on its own.
 */
TEST(Temp, InitPreparesTheAdc)
{
    testHwPlatformReset();
    temp_init();

    EXPECT_EQ(ADC0.CTRLA, static_cast<std::uint8_t>(ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc));
    EXPECT_EQ(VREF.ADC0REF, VREF_REFSEL_VDD_gc);
    EXPECT_EQ(PORTD.PIN4CTRL, PORT_ISC_INPUT_DISABLE_gc);
}

/**
 * @brief Test that the sensor is read from the analog input it is wired to.
 */
TEST(Temp, ReadUsesTheSensorsAnalogInput)
{
    testHwPlatformReset();
    temp_init();
    prepareVoltage(750U);
    (void)temp_read();

    EXPECT_EQ(ADC0.MUXPOS, ADC_MUXPOS_AIN4_gc);
}

/**
 * @brief Test that the sensor voltage is converted to degrees.
 *
 *        The sensor reports 500 mV at 0 degrees and 10 mV per degree above that, so 750 mV is
 *        room temperature and 1500 mV is boiling.
 */
TEST(Temp, ReadConvertsTheSensorVoltageToDegrees)
{
    testHwPlatformReset();
    temp_init();

    prepareVoltage(500U);
    EXPECT_EQ(temp_read(), 0);

    prepareVoltage(750U);
    EXPECT_EQ(temp_read(), 25);

    prepareVoltage(1500U);
    EXPECT_EQ(temp_read(), 100);
}

/**
 * @brief Test that temperatures below zero are reported as negative.
 *
 *        Everything the ADC hands over is unsigned, so a driver subtracting the sensor's offset
 *        in unsigned arithmetic reports a freezing room as roughly 6500 degrees instead.
 */
TEST(Temp, ReadHandlesTemperaturesBelowZero)
{
    testHwPlatformReset();
    temp_init();

    prepareVoltage(250U);
    EXPECT_EQ(temp_read(), -25);

    prepareVoltage(400U);
    EXPECT_EQ(temp_read(), -10);

    prepareVoltage(495U);
    EXPECT_EQ(temp_read(), -1);
}

/**
 * @brief Test that a reading is rounded to the nearest degree, in both directions.
 */
TEST(Temp, ReadRoundsToTheNearestDegree)
{
    testHwPlatformReset();
    temp_init();

    prepareVoltage(754U);
    EXPECT_EQ(temp_read(), 25);

    prepareVoltage(756U);
    EXPECT_EQ(temp_read(), 26);

    prepareVoltage(246U);
    EXPECT_EQ(temp_read(), -25);

    prepareVoltage(244U);
    EXPECT_EQ(temp_read(), -26);
}

/**
 * @brief Test what a failed conversion reports.
 *
 *        A conversion that never finishes reads as zero millivolts, which the sensor's scale
 *        makes -50 degrees. That is a plausible looking reading rather than an obvious error,
 *        so it is pinned down here to document it.
 */
TEST(Temp, FailedConversionReadsAsMinusFifty)
{
    testHwPlatformReset();
    temp_init();
    ADC0.RES = 1000U;

    EXPECT_EQ(temp_read(), -50);
}

/**
 * @brief Test that reading the sensor doesn't drive any I/O pin.
 */
TEST(Temp, ReadDoesNotDriveAnyPin)
{
    testHwPlatformReset();
    temp_init();
    prepareVoltage(750U);
    (void)temp_read();

    EXPECT_EQ(PORTD.DIR, zero);
    EXPECT_EQ(PORTD.OUTSET, zero);
    EXPECT_EQ(PORTD.OUTCLR, zero);
}
} // namespace
