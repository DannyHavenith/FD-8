#include <unity.h>
#include "adc.h"
#include "mapper.h"
#include <iostream>

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

    // At rest, the pot value should be near 255. Actually it gets quite a bit
    // far from that due to unknown effects (or just undiscovered mapping bugs).
    adc.test_set(kAdcRest);

    TEST_ASSERT_GREATER_OR_EQUAL(180, mapper.read_scaled_pedal(adc));
}

void test_pushNorth(void) {
    testFullPush(kAdcFullPushNorth);
}

void test_pushSouth(void) {
    testFullPush(kAdcFullPushSouth);
}

void dumpMappingTable(uint16_t pushValue) {
    adc adc;
    PedalMapper<> mapper;

    adc.test_set(kAdcRest);
    mapper.init_pedal_calibration(adc);

    // simulate a full push down to do self-calibration
    adc.test_set(pushValue);
    mapper.read_scaled_pedal(adc);
    // the mapper is now calibrated and ready for business.

    int direction = (pushValue > kAdcRest) ? 1 : -1;
    for (int i = kAdcRest; i != pushValue; i += direction) {
        adc.test_set(i);
        std::cout << "adc=" << i << " mapped=" << +mapper.read_scaled_pedal(adc) << std::endl;
    }
    adc.test_set(kAdcRest);
}

void dump_northTable(void) {
    std::cout << "Mapping table, north direction (foot push decreases value)" << std::endl;
    dumpMappingTable(kAdcFullPushNorth);
}

void dump_southTable(void) {
    std::cout << "Mapping table, south direction (foot push increases value)" << std::endl;
    dumpMappingTable(kAdcFullPushSouth);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_pushNorth);
    RUN_TEST(test_pushSouth);
    RUN_TEST(dump_northTable);
    RUN_TEST(dump_southTable);
    UNITY_END();
}
