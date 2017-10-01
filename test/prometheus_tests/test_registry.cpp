#include "ThinPrometheus.h"
#include "unity.h"

#ifdef UNIT_TEST

void test_registry_creates_metrics_only_once(void) {
    Registry registry;
    auto &gauge = registry.gauge("metric_name", "description text");
    
    gauge.set(10);

    registry.gauge("metric_name", "description text").increment(20.0);
    
    TEST_ASSERT_EQUAL_STRING("# HELP metric_name description text\n# TYPE metric_name gauge\nmetric_name 30\n", registry.represent().c_str());
}

void test_registry_represents_multiple_metrics(void) {
    Registry registry;
    registry.gauge("gauge_first" , "").set(10);
    registry.gauge("gauge_second" , "");
    registry.gauge("gauge_third" , "", {}, {{"label", "value"}});
    registry.counter("counter_first" , "").increment(10);

    TEST_ASSERT_EQUAL_STRING("# HELP counter_first \n"
                             "# TYPE counter_first counter\n"  
                             "counter_first 10\n"  
                             "# HELP gauge_first \n" 
                             "# TYPE gauge_first gauge\n" 
                             "gauge_first 10\n" 
                             "# HELP gauge_second \n" 
                             "# TYPE gauge_second gauge\n" 
                             "gauge_second 0\n" 
                             "# HELP gauge_third \n" 
                             "# TYPE gauge_third gauge\n"
                             "gauge_third{\"label\"=\"value\"} 0\n", registry.represent().c_str());
}

void runner_test_registry(void){
    UNITY_BEGIN();

    RUN_TEST(test_registry_creates_metrics_only_once);
    RUN_TEST(test_registry_represents_multiple_metrics);

    UNITY_END();
}

#endif