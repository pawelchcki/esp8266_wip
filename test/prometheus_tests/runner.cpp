#ifdef UNIT_TEST

void runner_test_gauge_counter(void);
void runner_test_registry(void);    

int main(int argc, char **argv) {
    runner_test_gauge_counter();
    runner_test_registry();

    return 0;
}

#endif