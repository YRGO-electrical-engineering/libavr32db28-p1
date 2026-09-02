/**
 * @brief Tests for the mocked hardware platform.
 *
 *        These verify the test wiring itself: that the C mock links into the C++ test binary,
 *        that the peripheral instances are writable, that testHwPlatformReset clears them, and
 *        that delay_ms is recorded rather than actually waited out.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"

#include "yrgo/test/test.h"

namespace
{
constexpr std::uint8_t zero{0U};

/** Durations used to check that delays are recorded in the order they were requested. */
constexpr std::uint16_t firstDelayMs{5U};
constexpr std::uint16_t secondDelayMs{7U};

/** A delay far longer than the test suite could afford to actually sleep for. */
constexpr std::uint16_t longDelayMs{60000U};

/** Positions in the list of recorded delays, and one far beyond anything the platform keeps. */
constexpr std::uint16_t firstDelay{0U};
constexpr std::uint16_t secondDelay{1U};
constexpr std::uint16_t unrecordedDelay{1000U};

/** Numbers of recorded delays the tests expect to find. */
constexpr std::uint16_t oneDelay{1U};
constexpr std::uint16_t twoDelays{2U};

/** Counter top value producing a one millisecond tick at 4 MHz with a /2 prescaler. */
constexpr std::uint16_t timerPeriod{1999U};

/** An arbitrary non-zero conversion, written to check that a reset clears the register. */
constexpr std::uint16_t someConversion{1234U};
} // namespace

/**
 * @brief Test that the mocked registers start out cleared.
 */
TEST(HwPlatform, ResetClearsRegisters)
{
    PORTA.DIR      = PIN7_bm;
    PORTC.OUTSET   = PIN0_bm;
    PORTD.IN       = PIN1_bm;
    PORTF.PIN0CTRL = PORT_PULLUPEN_bm;

    testHwPlatformReset();

    EXPECT_EQ(PORTA.DIR, zero);
    EXPECT_EQ(PORTC.OUTSET, zero);
    EXPECT_EQ(PORTD.IN, zero);
    EXPECT_EQ(PORTF.PIN0CTRL, zero);
}

/**
 * @brief Test that the port registers behave as independent storage.
 *
 *        The mock does not propagate OUTSET into OUT the way silicon does, which is why the
 *        driver tests assert on the exact register a driver wrote.
 */
TEST(HwPlatform, PortRegistersAreIndependent)
{
    testHwPlatformReset();

    PORTA.OUTSET = PIN7_bm;
    PORTA.OUTCLR = PIN0_bm;

    EXPECT_EQ(PORTA.OUTSET, PIN7_bm);
    EXPECT_EQ(PORTA.OUTCLR, PIN0_bm);
    EXPECT_EQ(PORTA.OUT, zero);
    EXPECT_EQ(PORTA.IN, zero);
}

/**
 * @brief Test that the ports are distinct objects.
 */
TEST(HwPlatform, PortsAreDistinct)
{
    testHwPlatformReset();

    PORTC.PIN0CTRL = PORT_PULLUPEN_bm;

    EXPECT_EQ(PORTC.PIN0CTRL, PORT_PULLUPEN_bm);
    EXPECT_EQ(PORTA.PIN0CTRL, zero);
    EXPECT_EQ(PORTD.PIN0CTRL, zero);
    EXPECT_EQ(PORTF.PIN0CTRL, zero);
}

/**
 * @brief Test that delays are recorded in the order they were requested.
 */
TEST(HwPlatform, DelaysAreRecorded)
{
    testHwPlatformReset();

    delay_ms(firstDelayMs);
    delay_ms(secondDelayMs);

    EXPECT_EQ(testDelayCount(), twoDelays);
    EXPECT_EQ(testDelayAt(firstDelay), firstDelayMs);
    EXPECT_EQ(testDelayAt(secondDelay), secondDelayMs);
}

/**
 * @brief Test that a delay returns immediately instead of sleeping.
 *
 *        A minute of real sleep would stall the suite, so the test completing at all is the
 *        assertion; the duration still has to be recorded faithfully.
 */
TEST(HwPlatform, DelaysDoNotSleep)
{
    testHwPlatformReset();

    delay_ms(longDelayMs);

    EXPECT_EQ(testDelayCount(), oneDelay);
    EXPECT_EQ(testDelayAt(firstDelay), longDelayMs);
}

/**
 * @brief Test that resetting the platform discards the recorded delays.
 */
TEST(HwPlatform, ResetClearsRecordedDelays)
{
    testHwPlatformReset();
    delay_ms(firstDelayMs);
    testHwPlatformReset();

    EXPECT_EQ(testDelayCount(), zero);
    EXPECT_EQ(testDelayAt(firstDelay), zero);
}

/**
 * @brief Test that reading a delay that was never recorded is harmless.
 */
TEST(HwPlatform, ReadingAnUnrecordedDelayReturnsZero)
{
    testHwPlatformReset();
    delay_ms(firstDelayMs);

    EXPECT_EQ(testDelayAt(secondDelay), zero);
    EXPECT_EQ(testDelayAt(unrecordedDelay), zero);
}

/**
 * @brief Test that resetting the platform clears the analog and timer peripherals too.
 */
TEST(HwPlatform, ResetClearsTheAnalogAndTimerPeripherals)
{
    ADC0.MUXPOS    = ADC_MUXPOS_AIN4_gc;
    ADC0.RES       = someConversion;
    TCB0.CCMP      = timerPeriod;
    TCB2.CTRLA     = TCB_ENABLE_bm;
    VREF.ADC0REF   = VREF_REFSEL_VDD_gc;
    PORTD.PIN7CTRL = PORT_PULLUPEN_bm;

    testHwPlatformReset();

    EXPECT_EQ(ADC0.MUXPOS, zero);
    EXPECT_EQ(ADC0.RES, zero);
    EXPECT_EQ(TCB0.CCMP, zero);
    EXPECT_EQ(TCB2.CTRLA, zero);
    EXPECT_EQ(VREF.ADC0REF, zero);
    EXPECT_EQ(PORTD.PIN7CTRL, zero);
}

/**
 * @brief Test that every pin of a port has its own control register.
 *
 *        The analog peripherals sit on PD4 - PD7, so the drivers configure pins the LED and
 *        switch drivers never touched.
 */
TEST(HwPlatform, EveryPinHasItsOwnControlRegister)
{
    testHwPlatformReset();

    PORTD.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTD.PIN7CTRL = PORT_PULLUPEN_bm;

    EXPECT_EQ(PORTD.PIN4CTRL, PORT_ISC_INPUT_DISABLE_gc);
    EXPECT_EQ(PORTD.PIN7CTRL, PORT_PULLUPEN_bm);
    EXPECT_EQ(PORTD.PIN5CTRL, zero);
    EXPECT_EQ(PORTD.PIN6CTRL, zero);
}

/**
 * @brief Test that the three timer circuits are distinct objects.
 */
TEST(HwPlatform, TimerCircuitsAreDistinct)
{
    testHwPlatformReset();

    TCB1.CCMP = timerPeriod;

    EXPECT_EQ(TCB1.CCMP, timerPeriod);
    EXPECT_EQ(TCB0.CCMP, zero);
    EXPECT_EQ(TCB2.CCMP, zero);
}

/**
 * @brief Test that the ADC result register holds a full twelve bit conversion.
 *
 *        A conversion runs from 0 to 4095, which does not fit in the eight bits most of the
 *        other registers are.
 */
TEST(HwPlatform, AdcResultHoldsATwelveBitValue)
{
    constexpr std::uint16_t fullScale{4095U};

    testHwPlatformReset();
    ADC0.RES = fullScale;

    EXPECT_EQ(ADC0.RES, fullScale);
}

/**
 * @brief Test that the interrupt flags are plain storage rather than write-one-to-clear.
 *
 *        On the real device a driver clears a flag by writing a one to it. The mock keeps
 *        whatever is written, so the flag would look stuck. Tests therefore lower the flags
 *        themselves; this test pins that behaviour down, since the driver tests depend on it.
 */
TEST(HwPlatform, InterruptFlagsKeepWhateverIsWritten)
{
    testHwPlatformReset();

    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTFLAGS = TCB_CAPT_bm;
    EXPECT_EQ(TCB0.INTFLAGS, TCB_CAPT_bm);

    ADC0.INTFLAGS = ADC_RESRDY_bm;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    EXPECT_EQ(ADC0.INTFLAGS, ADC_RESRDY_bm);

    TCB0.INTFLAGS = zero;
    ADC0.INTFLAGS = zero;
    EXPECT_EQ(TCB0.INTFLAGS, zero);
    EXPECT_EQ(ADC0.INTFLAGS, zero);
}
