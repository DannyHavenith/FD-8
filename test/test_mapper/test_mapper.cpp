#include <unity.h>
#include "adc.h"
#include "mapper.h"

adc adc;
PedalMapper mapper;

void setUp(void) {
    mapper.init_pedal_calibration(adc);
}

void tearDown(void) {
    // clean stuff up here
}

void test_mapper(void) {
    mapper.read_scaled_pedal(adc);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_mapper);
    UNITY_END();
}
