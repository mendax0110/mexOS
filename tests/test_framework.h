#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include "../shared/types.h"

/**
 * @brief Test result code
 */
#define TEST_PASS 0
#define TEST_FAIL 1
#define TEST_SKIP 2

/**
 * @brief Test stats structure \struct test_stats
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
 * @brief Test case structure \struct test_case
 */
struct test_case
{
    const char* name;
    test_func_t func;
};

/**
 * @brief Test suite structure \struct test_suite
 */
struct test_suite
{
    const char* name;
    struct test_case* cases;
    uint32_t count;
};

/**
 * @brief Test value types enumeration \enum test_value_types
 */
enum test_value_types
{
    TEST_VALUE_UNKNOWN,
    TEST_VALUE_U32,
    TEST_VALUE_U64,
    TEST_VALUE_S32,
    TEST_VALUE_S64,
    TEST_VALUE_F32,
    TEST_VALUE_F64,
    TEST_VALUE_PTR,
};

/**
 * @brief Test value structure \struct test_value
 */
struct test_value
{
    enum test_value_types type;

    /**
     * @brief Union of possible test value types \union anonymous
     */
    union
    {
        uint32_t u32;
        uint64_t u64;
        int32_t s32;
        int64_t s64;
        float32_t f32;
        float64_t f64;
        uintptr_t ptr;
    };
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
 * @brief Create a test value from a uint32_t
 * @param value The value to create
 * @return A test_value structure
 */
struct test_value test_make_value_u32(uint32_t value);

/**
 * @brief Create a test value from a uint64_t
 * @param value The value to create
 * @return A test_value structure
 */
struct test_value test_make_value_u64(uint64_t value);

/**
 * @brief Create a test value from a int32_t
 * @param value The value to create
 * @return A test_value structure
 */
struct test_value test_make_value_s32(int32_t value);

/**
 * @brief Create a test value from a int64_t
 * @param value The value to create
 * @return A test_value structure
 */
struct test_value test_make_value_s64(int64_t value);

/**
 * @brief Create a test value from a float32_t
 * @param value The value to create
 * @return A test_value structure
 */
struct test_value test_make_value_f32(float32_t value);

/**
 * @brief Create a test value from a float64_t
 * @param value The value to create
 * @return A test_value structure
 */
struct test_value test_make_value_f64(float64_t value);

/**
 * @brief Create a test value from a pointer
 * @param value The value to create
 * @return A test_value structure
 */
struct test_value test_make_value_ptr(const void* value);

/**
 * @brief Assert equality failure handler
 * @param file The source file where the assertion failed
 * @param line The line number of the assertion
 * @param equal Whether the assertion was for equality or inequality
 * @param actual_expr The expression for the actual value
 * @param expected_expr The expression for the expected value
 * @param actual The actual value
 * @param expected The expected value
 */
void test_assert_print_fail(const char* file, int line, bool equal, const char* actual_expr, const char* expected_expr, struct test_value actual, struct test_value expected);

/**
 * @brief Test case macros
 */
#define TEST_ASSERT(expr)                                   \
    do                                                      \
    {                                                       \
        if (!(expr))                                        \
        {                                                   \
            test_assert_fail(__FILE__, __LINE__, #expr);    \
            return TEST_FAIL;                               \
        }                                                   \
    } while (0)

/**
 * @brief Helper macro to create a test value from a given value
 * @param value The value to create a test_value from
 */
#define TEST_VALUE(value)                           \
    GENERIC((value),                                \
        unsigned char: test_make_value_u32,         \
        unsigned short: test_make_value_u32,        \
        unsigned int: test_make_value_u32,          \
        unsigned long long: test_make_value_u64,    \
        signed char: test_make_value_s32,           \
        signed short: test_make_value_s32,          \
        signed int: test_make_value_s32,            \
        signed long long: test_make_value_s64,      \
        float: test_make_value_f32,                 \
        double: test_make_value_f64,                \
        default: test_make_value_ptr                \
    )(value)

/**
 * @brief Helper macro to compare two values are equal
 * @param a The first value to compare
 * @param b The second value to compare
 */
#define TEST_ASSERT_EQ(a, b)                    \
    do                                          \
    {                                           \
        const __typeof__(a) _actual = (a);      \
        const __typeof__(b) _expected = (b);    \
                                                \
        if (_actual != _expected)               \
        {                                       \
            test_assert_print_fail(             \
                __FILE__,                       \
                __LINE__,                       \
                true,                           \
                #a,                             \
                #b,                             \
                TEST_VALUE(_actual),            \
                TEST_VALUE(_expected));         \
            return TEST_FAIL;                   \
        }                                       \
    } while (0)

/**
 * @brief Helper macro to compare that two values are not equal
 * @param a The first value to compare
 * @param b The second value to compare
 */
#define TEST_ASSERT_NEQ(a, b)                               \
    do                                                      \
    {                                                       \
        const __typeof__(a) _actual = (a);                  \
        const __typeof__(b) _expected = (b);                \
        if (_actual == _expected)                           \
        {                                                   \
            test_assert_print_fail(                         \
                __FILE__,                                   \
                __LINE__,                                   \
                false,                                      \
                #a,                                         \
                #b,                                         \
                TEST_VALUE(_actual),                        \
                TEST_VALUE(_expected));                     \
            return TEST_FAIL;                               \
        }                                                   \
    } while (0)

/**
 * @brief Helper macro to asser that a pointer is null
 * @param ptr The pointer to check
 */
#define TEST_ASSERT_NULL(ptr) \
    TEST_ASSERT((ptr) == NULL)

/**
 * @brief Helper macro to asser that a pointer is null
 * @param ptr The pointer to check
 */
#define TEST_ASSERT_NOT_NULL(ptr) \
    TEST_ASSERT((ptr) != NULL)

/**
 * @brief Helper macro to assert that an expression is true
 * @param expr The expression to evaluate
 */
#define TEST_ASSERT_TRUE(expr) \
    TEST_ASSERT((expr))

/**
 * @brief Helper macro to assert that an expression is false
 * @param expr The expression to evaluate
 */
#define TEST_ASSERT_FALSE(expr) \
    TEST_ASSERT(!(expr))

/**
 * @brief Helper macro to check that val a is greater than val b
 * @param a The value which must be bigger
 * @param b The value which must be smaller
 */
#define TEST_ASSERT_GT(a, b) \
    TEST_ASSERT((a) > (b))

/**
 * @brief Helper macro to check that val a is greater than or equal to val b
 * @param a The value which must be bigger or equal
 * @param b The value which must be smaller or equal
 */
#define TEST_ASSERT_GE(a, b) \
    TEST_ASSERT((a) >= (b))

/**
 * @brief Helper macro to check that val a is less than val b
 * @param a The value which must be smaller
 * @param b The value which must be bigger
 */
#define TEST_ASSERT_LT(a, b) \
    TEST_ASSERT((a) < (b))

/**
 * @brief Helper macro to check that val a is less than or equal to val b
 * @param a The value which must be smaller or equal
 * @param b The value which must be bigger or equal
 */
#define TEST_ASSERT_LE(a, b) \
    TEST_ASSERT((a) <= (b))

/**
 * @brief Helper macro to check that string a is equal to string b
 * @param a The first string to compare
 * @param b The second string to compare
 */
#define TEST_ASSERT_STR_EQ(a, b) \
    TEST_ASSERT(strcmp((a), (b)) == 0)

/**
 * @brief Helper macro to check that memory regions a and b are equal
 * @param a The first memory region to compare
 * @param b The second memory region to compare
 * @param len The length of the memory regions to compare
 */
#define TEST_ASSERT_MEM_EQ(a, b, len) \
    TEST_ASSERT(memcmp((a), (b), (len)) == 0)

/**
 * @brief Define a test case
 * @brief The test case
 */
#define TEST_CASE(name) \
    static int test_##name(void)

/**
 * @brief Skips a test case
 * @param name The test case to skip
 */
#define TEST_CASE_IGNORE(name)                                  \
    MAYBE_UNUSED static int test_##name##_disabled(void);       \
    static int test_##name(void)                                \
    {                                                           \
        return TEST_SKIP;                                       \
    }                                                           \
    MAYBE_UNUSED static int test_##name##_disabled(void)

/**
 * @brief Create a test case entry for suite
 * @brief name The test case
 */
#define TEST_ENTRY(name) \
    { #name, test_##name }

/**
 * @brief End marker for a test suite
 */
#define TEST_SUITE_END \
    { NULL, NULL }

/**
 * @brief Helper to create test suite
 * @param suite_name The suite name
 * @param case_array The array
 */
#define TEST_SUITE(suite_name, case_array)          \
{                                                   \
    .name = (suite_name),                           \
    .cases = (case_array),                          \
    .count = ARRAY_SIZE(case_array) - 1             \
}

#endif // TEST_FRAMEWORK_H