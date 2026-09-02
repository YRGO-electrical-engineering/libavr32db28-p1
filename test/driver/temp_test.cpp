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

/** Sensor voltage at zero degrees. It reports ten millivolts per degree above that. */
constexpr std::uint16_t zeroDegreesMv{500U};
constexpr std::int16_t zeroDegreesC{0};

/** A room temperature reading, written wherever the exact temperature is beside the point. */
constexpr std::uint16_t roomTemperatureMv{750U};
constexpr std::int16_t roomTemperatureC{25};

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
    prepareVoltage(roomTemperatureMv);
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
    constexpr std::uint16_t boilingMv{1500U};
    constexpr std::int16_t boilingC{100};

    testHwPlatformReset();
    temp_init();

    prepareVoltage(zeroDegreesMv);
    EXPECT_EQ(temp_read(), zeroDegreesC);

    prepareVoltage(roomTemperatureMv);
    EXPECT_EQ(temp_read(), roomTemperatureC);

    prepareVoltage(boilingMv);
    EXPECT_EQ(temp_read(), boilingC);
}

/**
 * @brief Test that temperatures below zero are reported as negative.
 *
 *        Everything the ADC hands over is unsigned, so a driver subtracting the sensor's offset
 *        in unsigned arithmetic reports a freezing room as roughly 6500 degrees instead.
 */
TEST(Temp, ReadHandlesTemperaturesBelowZero)
{
    constexpr std::uint16_t freezerMv{250U};
    constexpr std::int16_t freezerC{-25};
    constexpr std::uint16_t coldRoomMv{400U};
    constexpr std::int16_t coldRoomC{-10};
    constexpr std::uint16_t justBelowZeroMv{495U};
    constexpr std::int16_t justBelowZeroC{-1};

    testHwPlatformReset();
    temp_init();

    prepareVoltage(freezerMv);
    EXPECT_EQ(temp_read(), freezerC);

    prepareVoltage(coldRoomMv);
    EXPECT_EQ(temp_read(), coldRoomC);

    prepareVoltage(justBelowZeroMv);
    EXPECT_EQ(temp_read(), justBelowZeroC);
}

/**
 * @brief Test that a reading is rounded to the nearest degree, in both directions.
 */
TEST(Temp, ReadRoundsToTheNearestDegree)
{
    // Voltages a little either side of the halfway point between two degrees, above and below
    // zero, together with the degree each one is expected to land on.
    constexpr std::uint16_t belowMidpointMv{754U};
    constexpr std::int16_t belowMidpointC{25};
    constexpr std::uint16_t aboveMidpointMv{756U};
    constexpr std::int16_t aboveMidpointC{26};
    constexpr std::uint16_t belowNegativeMidpointMv{246U};
    constexpr std::int16_t belowNegativeMidpointC{-25};
    constexpr std::uint16_t aboveNegativeMidpointMv{244U};
    constexpr std::int16_t aboveNegativeMidpointC{-26};

    testHwPlatformReset();
    temp_init();

    prepareVoltage(belowMidpointMv);
    EXPECT_EQ(temp_read(), belowMidpointC);

    prepareVoltage(aboveMidpointMv);
    EXPECT_EQ(temp_read(), aboveMidpointC);

    prepareVoltage(belowNegativeMidpointMv);
    EXPECT_EQ(temp_read(), belowNegativeMidpointC);

    prepareVoltage(aboveNegativeMidpointMv);
    EXPECT_EQ(temp_read(), aboveNegativeMidpointC);
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
    // A plausible result left in the register, which the driver never gets to read: the
    // conversion complete flag is not raised, so the read times out.
    constexpr std::uint16_t unreadResult{1000U};
    constexpr std::int16_t failedReadingC{-50};

    testHwPlatformReset();
    temp_init();
    ADC0.RES = unreadResult;

    EXPECT_EQ(temp_read(), failedReadingC);
}

/**
 * @brief Test that reading the sensor doesn't drive any I/O pin.
 */
TEST(Temp, ReadDoesNotDriveAnyPin)
{
    testHwPlatformReset();
    temp_init();
    prepareVoltage(roomTemperatureMv);
    (void)temp_read();

    EXPECT_EQ(PORTD.DIR, zero);
    EXPECT_EQ(PORTD.OUTSET, zero);
    EXPECT_EQ(PORTD.OUTCLR, zero);
}
} // namespace
