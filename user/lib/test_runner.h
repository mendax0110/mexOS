#ifndef USER_TEST_RUNNER_H
#define USER_TEST_RUNNER_H

#include "test_framework.h"

/**
 * @brief Struct to represent a test registry entry \struct test_registry_entry
 */
typedef struct
{
    const char* name;
    const char* description;
    struct test_suite* (*get_suite)(void);
} test_registry_entry;

/**
 * @brief Runs all registered test suites and their test cases.
 */
void run_all_tests(void);

/**
 * @brief Runs all registered test suites and their test cases with console output.
 */
void run_all_tests_console(void);

/**
 * @brief Runs a specific test suite by name with console output.
 * @param name The name of the test suite to run
 */
void run_suite_console(const char* name);

/**
 * @brief Runs a specific test case by suite name and test case name with console output.
 * @param suite_name The name of the test suite
 * @param test_name The name of the test case
 */
void run_single_test_console(const char* suite_name, const char* test_name);

/**
 * @brief Retrieves a test suite by its name.
 * @param name The name of the test suite
 * @return Pointer to the test suite, or NULL if not found
 */
struct test_suite* test_get_suite_by_name(const char* name);

/**
 * @brief Runs a specific test case by suite name and test case name.
 * @param suite_name The name of the test suite
 * @param test_name The name of the test case
 * @return 0 on success, otherwise failure
 */
int test_run_single(const char* suite_name, const char* test_name);

/**
 * @brief Retrieves the test registry and its count.
 * @param count Pointer to a size_t variable to store the number of test suites
 * @return Pointer to the array of test registry entries
 */
const test_registry_entry* test_get_registry(size_t* count);

/**
 * @brief Counts the number of test cases in a given test suite.
 * @param cases Pointer to the array of test cases
 * @return The number of test cases in the array
 */
uint32_t test_case_count(const struct test_case* cases);

#endif
