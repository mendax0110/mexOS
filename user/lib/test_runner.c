#include "test_runner.h"

struct test_suite* test_user_helpers_get_suite(void);

static const test_registry_entry test_registry[] =
{
    { "helpers", "Userspace helper tests", test_user_helpers_get_suite },
};

static const size_t TEST_SUITE_COUNT = sizeof(test_registry) / sizeof(test_registry[0]);

struct test_suite* test_get_suite_by_name(const char* name)
{
    for (size_t i = 0; i < TEST_SUITE_COUNT; i++)
    {
        if (user_strcmp(name, test_registry[i].name) == 0)
        {
            return test_registry[i].get_suite();
        }
    }

    return NULL;
}

const test_registry_entry* test_get_registry(size_t* count)
{
    *count = TEST_SUITE_COUNT;
    return test_registry;
}

int test_run_single(const char* suite_name, const char* test_name)
{
    struct test_suite* suite = test_get_suite_by_name(suite_name);
    if (!suite)
    {
        return -1;
    }

    for (uint32_t i = 0; i < suite->count; i++)
    {
        if (suite->cases[i].name && user_strcmp(suite->cases[i].name, test_name) == 0)
        {
            return test_run_case(suite->cases[i].name, suite->cases[i].func);
        }
    }

    return -1;
}

void run_all_tests(void)
{
    test_init();
    for (size_t i = 0; i < TEST_SUITE_COUNT; i++)
    {
        test_run_suite(test_registry[i].get_suite());
    }
    test_summary();
}

void run_all_tests_console(void)
{
    run_all_tests();
}

void run_suite_console(const char* name)
{
    test_init_console();
    struct test_suite* suite = test_get_suite_by_name(name);
    if (suite)
    {
        test_run_suite(suite);
        test_summary();
    }
}

void run_single_test_console(const char* suite_name, const char* test_name)
{
    test_init_console();
    test_run_single(suite_name, test_name);
    test_summary();
}

uint32_t test_case_count(const struct test_case* cases)
{
    uint32_t count = 0;
    while (cases[count].name != NULL)
    {
        count++;
    }
    return count;
}
