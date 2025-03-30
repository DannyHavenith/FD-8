#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_foo(void) {
    TEST_ASSERT_EQUAL(2, 2);
}

void test_bar(void) {
    TEST_ASSERT_EQUAL(2, 3);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_foo);
    RUN_TEST(test_bar);
    UNITY_END();
}
