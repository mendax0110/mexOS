#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "test_framework.h"

/**
 * @brief Stores the test runners information \struct test_registry_entry
 */
typedef struct
{
    const char* name;
    const char* description;
    struct test_suite* (*get_suite)(void);
} test_registry_entry;

/**
 * @brief Run all kernel unit tests (output to vterm)
 */
void run_all_tests(void);

/**
 * @brief Run all kernel unit tests (output to console)
 */
void run_all_tests_console(void);

/**
 * @brief Run a specific test suite (output to console)
 * @param name The name of the suite (pmm, heap, string, fs, ipc, sched)
 */
void run_suite_console(const char* name);

/**
 * @brief Run a single test case (output to console)
 * @param suite_name The name of the suite
 * @param test_name The name of the test case
 */
void run_single_test_console(const char* suite_name, const char* test_name);

/**
 * @brief Get a test suite by name
 * @param name The name of the suite
 * @return Pointer to the test suite, or NULL if not found
 */
struct test_suite* test_get_suite_by_name(const char* name);

/**
 * @brief Run a single test from a suite
 * @param suite_name The name of the suite
 * @param test_name The name of the test
 * @return TEST_PASS, TEST_FAIL, TEST_SKIP, or -1 if not found
 */
int test_run_single(const char* suite_name, const char* test_name);

/**
 * @brief Getter for the test registry
 * @param count the count
 * @return A ptr
 */
const test_registry_entry* test_get_registry(size_t* count);

/**
 * @brief Helper to determine the test case count
 * @param cases the test cases
 * @return the count
 */
uint32_t test_case_count(const struct test_case* cases);

#endif
