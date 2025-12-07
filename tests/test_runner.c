#include "test_runner.h"
#include "test_framework.h"
#include "mm/test_pmm.h"
#include "mm/test_heap.h"
#include "core/test_string.h"
#include "core/test_fs.h"
#include "ipc/test_ipc.h"
#include "sched/test_sched.h"
#include "../kernel/include/string.h"

struct test_suite* test_get_suite_by_name(const char* name)
{
    if (strcmp(name, "pmm") == 0)
    {
        return test_pmm_get_suite();
    }
    if (strcmp(name, "heap") == 0)
    {
        return test_heap_get_suite();
    }
    if (strcmp(name, "string") == 0)
    {
        return test_string_get_suite();
    }
    if (strcmp(name, "fs") == 0)
    {
        return test_fs_get_suite();
    }
    if (strcmp(name, "ipc") == 0)
    {
        return test_ipc_get_suite();
    }
    if (strcmp(name, "sched") == 0)
    {
        return test_sched_get_suite();
    }
    return NULL;
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

    test_run_suite(test_string_get_suite());
    test_run_suite(test_pmm_get_suite());
    test_run_suite(test_heap_get_suite());
    test_run_suite(test_fs_get_suite());
    test_run_suite(test_ipc_get_suite());
    test_run_suite(test_sched_get_suite());

    test_summary();
}

void run_all_tests_console(void)
{
    test_init_console();

    test_run_suite(test_string_get_suite());
    test_run_suite(test_pmm_get_suite());
    test_run_suite(test_heap_get_suite());
    test_run_suite(test_fs_get_suite());
    test_run_suite(test_ipc_get_suite());
    test_run_suite(test_sched_get_suite());

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
