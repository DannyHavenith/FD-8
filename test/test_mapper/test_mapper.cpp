#include <unity.h>
#include "adc.h"
#include "mapper.h"

// Some ADC values sampled from real sensing (Allegro A1302)
const uint16_t kAdcRest = 508;
const uint16_t kAdcFullPushDelta = 45;
const uint16_t kAdcFullPushNorth = kAdcRest - kAdcFullPushDelta;
const uint16_t kAdcFullPushSouth = kAdcRest + kAdcFullPushDelta;

void setUp(void) {
}

void tearDown(void) {
    // clean stuff up here
}

void testFullPush(uint16_t pushValue) {
    adc adc;
    PedalMapper<> mapper;

    // start at some rest value to perform initial calibration
    adc.test_set(kAdcRest);
    mapper.init_pedal_calibration(adc);

    // simulate a full push down to do self-calibration
    adc.test_set(pushValue);
    mapper.read_scaled_pedal(adc);
    // the mapper is now calibrated and ready for business.

    // At rest, the pot value should be near 255 (not quite 255 because of
    // rounding errors)
    adc.test_set(kAdcRest);

    TEST_ASSERT_GREATER_OR_EQUAL(250, mapper.read_scaled_pedal(adc));
}

void test_pushNorth(void) {
    testFullPush(kAdcFullPushNorth);
}

void test_pushSouth(void) {
    testFullPush(kAdcFullPushSouth);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_pushNorth);
    RUN_TEST(test_pushSouth);
    UNITY_END();
}
