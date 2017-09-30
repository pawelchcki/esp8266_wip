#include "ThinPrometheus.h"
#include "unity.h"

#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void loop(){

}

void setup(){

}

void test_function_calculator_division(void) {
    Registry registry;
    auto gauge = registry.gauge("name", "description text");
    
    gauge.set(192.0);
    
    TEST_ASSERT_EQUAL(192.0, gauge.get());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
   
    RUN_TEST(test_function_calculator_division);
    UNITY_END();

    return 0;
}

#endif