#include "ThinPrometheus.h"
#include "unity.h"

#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void test_gauge_is_created_and_can_produce_simple_representation(void) {
    Registry registry;
    auto gauge = registry.gauge("metric_name", "description text");
    
    gauge.set(192.2);
    
    TEST_ASSERT_EQUAL(192.2, gauge.get());
    TEST_ASSERT_EQUAL_STRING("# HELP metric_name description text\n# TYPE metric_name gauge\nmetric_name 192.2\n", gauge.represent().c_str());
}

void test_gauge_defaults_to_0(void){
    Registry registry;
    auto gauge = registry.gauge("metric_name", "description text");

    TEST_ASSERT_EQUAL_STRING("# HELP metric_name description text\n# TYPE metric_name gauge\nmetric_name 0\n", gauge.represent().c_str());
}

void test_gauge_increments_by_set_amount(void){
    Registry registry;
    auto gauge = registry.gauge("metric_name", "description text");

    gauge.set(10);
    gauge.increment(20);

    TEST_ASSERT_EQUAL_STRING("# HELP metric_name description text\n# TYPE metric_name gauge\nmetric_name 30\n", gauge.represent().c_str());
}

void test_gauge_can_have_labels(void){
    Registry registry;
    auto gauge = registry.gauge("metric_name", "description text");
    
    gauge.set({{"a", "b"}}, 10);
    gauge.set({{"a", "c"}}, 20);

    gauge.increment({{"a", "b"}}, 20);

    TEST_ASSERT_EQUAL_STRING("# HELP metric_name description text\n# TYPE metric_name gauge\nmetric_name{\"a\"=\"b\"} 30\nmetric_name{\"a\"=\"c\"} 20\n", gauge.represent().c_str());    
}

void test_default_labels_can_be_changed(void) {
    Registry registry;
    auto gauge = registry.gauge("metric_name", "description text", {}, {{"a", "b"}});

    TEST_ASSERT_EQUAL_STRING("# HELP metric_name description text\n# TYPE metric_name gauge\nmetric_name{\"a\"=\"b\"} 0\n", gauge.represent().c_str());    
}

// TODO: test_gauge_can_not_have_different_set_of_labels

void runner_test_gauge_counter(){
    UNITY_BEGIN();
    
     RUN_TEST(test_gauge_is_created_and_can_produce_simple_representation);
     RUN_TEST(test_gauge_defaults_to_0);
     RUN_TEST(test_gauge_increments_by_set_amount);
     RUN_TEST(test_gauge_can_have_labels);
     RUN_TEST(test_default_labels_can_be_changed);
 
     UNITY_END();
}
#endif