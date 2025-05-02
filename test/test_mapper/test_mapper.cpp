#include <unity.h>
#include "adc.h"
#include "mapper.h"

adc adc;
PedalMapper mapper;

void setUp(void) {
}

void tearDown(void) {
    // clean stuff up here
}

void test_mapper(void) {
    // start at some rest value to perform initial calibration
    adc.test_set(508);
    mapper.init_pedal_calibration(adc);

    // simulate a full push down to do self-calibration
    adc.test_set(463);
    mapper.read_scaled_pedal(adc);
    // the mapper is now calibrated and ready for business.

    // At rest, the pot value should be near 255 (not quite 255 because of
    // rounding errors)
    adc.test_set(508);
    TEST_ASSERT_GREATER_OR_EQUAL_CHAR(250, mapper.read_scaled_pedal(adc));
}

int main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_mapper);
    UNITY_END();
}
