#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include "../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Test result code
 */
#define TEST_PASS 0
#define TEST_FAIL 1
#define TEST_SKIP 2

/**
 * @brief Test stats structure
 */
struct test_stats
{
    uint32_t total;
    uint32_t passed;
    uint32_t failed;
    uint32_t skipped;
};

/**
 * @brief Test case funcPtr
 */
typedef int (*test_func_t)(void);

/**
 * @brief Test case struture
 */
struct test_case
{
    const char* name;
    test_func_t func;
};

/**
 * @brief Test suite struture
 */
struct test_suite
{
    const char* name;
    struct test_case* cases;
    uint32_t count;
};

/**
 * @brief Init the test framework
 */
void test_init(void);

/**
 * @brief Initialize the test framework for console output
 */
void test_init_console(void);

/**
 * @brief Run a test case
 * @param name Test case name
 * @param func Test case function
 * @return A test result code
 */
int test_run_case(const char* name, test_func_t func);

/**
 * @brief Run a test suite
 * @param suite The test suite to run
 */
void test_run_suite(struct test_suite* suite);

/**
 * @brief Get test statistics
 * @return Pointer to test_stats structure
 */
struct test_stats* test_get_stats(void);

/**
 * @brief Print a summary of test results
 */
void test_summary(void);

/**
 * @brief Assert failure handler
 * @param file The source file where the assertion failed
 * @param line The line number of the assertion
 * @param expr The expression that failed
 */
void test_assert_fail(const char* file, int line, const char* expr);

/**
 * @brief Test case macros
 */
#define TEST_ASSERT(expr)                               \
    do                                                  \
    {                                                   \
        if (!(expr))                                    \
        {                                               \
            test_assert_fail(__FILE__, __LINE__, #expr); \
            return TEST_FAIL;                           \
        }                                               \
    } while (0)

#define TEST_ASSERT_EQ(a, b) \
    TEST_ASSERT((a) == (b))

#define TEST_ASSERT_NEQ(a, b) \
    TEST_ASSERT((a) != (b))

#define TEST_ASSERT_NULL(ptr) \
    TEST_ASSERT((ptr) == NULL)

#define TEST_ASSERT_NOT_NULL(ptr) \
    TEST_ASSERT((ptr) != NULL)

#define TEST_ASSERT_TRUE(expr) \
    TEST_ASSERT((expr))

#define TEST_ASSERT_FALSE(expr) \
    TEST_ASSERT(!(expr))

#define TEST_ASSERT_GT(a, b) \
    TEST_ASSERT((a) > (b))

#define TEST_ASSERT_GE(a, b) \
    TEST_ASSERT((a) >= (b))

#define TEST_ASSERT_LT(a, b) \
    TEST_ASSERT((a) < (b))

#define TEST_ASSERT_LE(a, b) \
    TEST_ASSERT((a) <= (b))

#define TEST_ASSERT_STR_EQ(a, b) \
    TEST_ASSERT(strcmp((a), (b)) == 0)

#define TEST_ASSERT_MEM_EQ(a, b, len) \
    TEST_ASSERT(memcmp((a), (b), (len)) == 0)

/**
 * @brief Define a test case
 */
#define TEST_CASE(name) \
    static int test_##name(void)

/**
 * @brief Create a test case entry for suite
 */
#define TEST_ENTRY(name) \
    { #name, test_##name }

/**
 * @brief End marker for a test suite
 */
#define TEST_SUITE_END \
    { NULL, NULL }

#ifdef __cplusplus
}
#endif

#endif // TEST_FRAMEWORK_H