#include "test_runner.h"
#include "test_framework.h"
#include "mm/test_pmm.h"
#include "mm/test_vmm.h"
#include "mm/test_heap.h"
#include "core/test_string.h"
#include "core/test_fs.h"
#include "ipc/test_ipc.h"
#include "sched/test_sched.h"
#include "types/test_types.h"
#include "rtc/test_rtc.h"
#include "types/test_rollback.h"
#include "stress/test_stress.h"
#include "../kernel/lib/string.h"

static const test_registry_entry test_registry[] =
{
    { "pmm",      "Physical Memory Manager tests", test_pmm_get_suite },
    { "vmm",      "Virtual Memory Manager tests",  test_vmm_get_suite },
    { "heap",     "Kernel Heap tests",             test_heap_get_suite },
    { "string",   "String Function tests",         test_string_get_suite },
    { "fs",       "Filesystem tests",              test_fs_get_suite },
    { "rtc",      "RTC Driver tests",              test_rtc_get_suite },
    { "ipc",      "Inter-Process Communication tests", test_ipc_get_suite },
    { "sched",    "Scheduler tests",               test_sched_get_suite },
    { "types",    "Types and Casts tests",         test_types_get_suite },
    { "rollback", "Rollback Mechanism tests",      test_rollback_get_suite },
    { "stress",   "Stress tests for kernel components", test_stress_get_suite }
};

#define TEST_SUITE_COUNT \
    (sizeof(test_registry) / sizeof(test_registry[0]))

struct test_suite* test_get_suite_by_name(const char* name)
{
    for (size_t i = 0; i < TEST_SUITE_COUNT; i++)
    {
        if (strcmp(name, test_registry[i].name) == 0)
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
        if (suite->cases[i].name && strcmp(suite->cases[i].name, test_name) == 0)
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
    test_init_console();

    for (size_t i = 0; i < TEST_SUITE_COUNT; i++)
    {
        test_run_suite(test_registry[i].get_suite());
    }

    test_summary();
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