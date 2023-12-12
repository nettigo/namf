#include "testable.h"
#include <unity.h>

void setUp() {};
void tearDown() {};
void test_hours(){
    TEST_ASSERT_TRUE(hourIsInRange(23,20,6));
    TEST_ASSERT_TRUE(hourIsInRange(23,20,23));
    TEST_ASSERT_FALSE(hourIsInRange(18,20,23));
    TEST_ASSERT_FALSE(hourIsInRange(18,20,16));
    TEST_ASSERT_FALSE(hourIsInRange(1,20,24));
}

int main( int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_hours);
    UNITY_END();
}