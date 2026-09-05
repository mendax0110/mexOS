#ifndef SHARED_TEST_FRAMEWORK_H
#define SHARED_TEST_FRAMEWORK_H

#include "types.h"
#include "compiler.h"

#ifndef TEST_FRAMEWORK_STRCMP
#error "TEST_FRAMEWORK_STRCMP must be defined before including shared/test_framework.h"
#endif

#ifndef TEST_FRAMEWORK_MEMCMP
#error "TEST_FRAMEWORK_MEMCMP must be defined before including shared/test_framework.h"
#endif

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
 * @brief Test function type definition \struct test_func_t
 */
typedef int (*test_func_t)(void);

/**
 * @brief Struct to represent the test_case \struct test_case
 */
struct test_case
{
    const char* name;
    test_func_t func;
};

/**
 * @brief Struct to represent the test_suite \struct test_suite
 */
struct test_suite
{
    const char* name;
    struct test_case* cases;
    uint32_t count;
};

/**
 * @brief Enum to represent the test_value_types \enum test_value_types
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
 * @brief Struct to represent the test_value \struct test_value
 */
struct test_value
{
    enum test_value_types type;
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
 * @brief Initializes the test framework.
 */
void test_init(void);

/**
 * @brief Initializes the test framework for console output.
 */
void test_init_console(void);

/**
 * @brief Runs a given test case
 * @param name The test case name
 * @param func The test function
 * @return 0 on success, otherwise failure
 */
int test_run_case(const char* name, test_func_t func);

/**
 * @brief Runs a given test suite
 * @param suite The test suite to run
 */
void test_run_suite(struct test_suite* suite);

/**
 * @brief Getter for the test stats
 * @return Pointer to the test stats structure
 */
struct test_stats* test_get_stats(void);

/**
 * @brief Prints a summary of the test results
 */
void test_summary(void);

/**
 * @brief Handles a failed assertion, printing the file, line, and expression that failed
 * @param file The file name where the assertion failed
 * @param line The line number where the assertion failed
 * @param expr The expression that failed
 */
void test_assert_fail(const char* file, int line, const char* expr);

/**
 * @brief Creates a test_value struct for a given uint32_t value
 * @param value The uint32_t value to wrap in a test_value struct
 * @return A test_value struct containing the given uint32_t value
 */
struct test_value test_make_value_u32(uint32_t value);

/**
 * @brief Creates a test_value struct for a given uint64_t value
 * @param value The uint64_t value to wrap in a test_value struct
 * @return A test_value struct containing the given uint64_t value
 */
struct test_value test_make_value_u64(uint64_t value);

/**
 * @brief Creates a test_value struct for a given int32_t value
 * @param value The int32_t value to wrap in a test_value struct
 * @return A test_value struct containing the given int32_t value
 */
struct test_value test_make_value_s32(int32_t value);

/**
 * @brief Creates a test_value struct for a given int64_t value
 * @param value The int64_t value to wrap in a test_value struct
 * @return A test_value struct containing the given int64_t value
 */
struct test_value test_make_value_s64(int64_t value);

/**
 * @brief Creates a test_value struct for a given float32_t value
 * @param value The float32_t value to wrap in a test_value struct
 * @return A test_value struct containing the given float32_t value
 */
struct test_value test_make_value_f32(float32_t value);

/**
 * @brief Creates a test_value struct for a given float64_t value
 * @param value The float64_t value to wrap in a test_value struct
 * @return A test_value struct containing the given float64_t value
 */
struct test_value test_make_value_f64(float64_t value);

/**
 * @brief Creates a test_value struct for a given pointer value
 * @param value The pointer value to wrap in a test_value struct
 * @return A test_value struct containing the given pointer value
 */
struct test_value test_make_value_ptr(const void* value);

/**
 * @brief Prints a failed assertion message with details about the actual and expected values
 * @param file The file name where the assertion failed
 * @param line The line number where the assertion failed
 * @param equal A boolean indicating if the assertion was for equality (true) or inequality (false)
 * @param actual_expr The string representation of the actual expression
 * @param expected_expr The string representation of the expected expression
 * @param actual The actual value wrapped in a test_value struct
 * @param expected The expected value wrapped in a test_value struct
 */
void test_assert_print_fail(const char* file, int line, bool equal,
                            const char* actual_expr, const char* expected_expr,
                            struct test_value actual, struct test_value expected);

/**
 * @brief Macro to assert a condition and handle failure
 * @param expr The expression to evaluate
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
 * @brief Macro to create a test_value struct based on the type of the value
 * @param value The value to wrap in a test_value struct
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
 * @brief Macro to assert equality between two values and handle failure
 * @param a The actual value
 * @param b The expected value
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
 * @brief Macro to assert inequality between two values and handle failure
 * @param a The actual value
 * @param b The expected value
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
 * @brief Macro to assert that a pointer is NULL
 * @param ptr The pointer to check
 */
#define TEST_ASSERT_NULL(ptr) \
    TEST_ASSERT((ptr) == NULL)

/**
 * @brief Macro to assert that a pointer is not NULL
 * @param ptr The pointer to check
 */
#define TEST_ASSERT_NOT_NULL(ptr) \
    TEST_ASSERT((ptr) != NULL)

/**
 * @brief Macro to assert that an expression is true
 * @param expr The expression to evaluate
 */
#define TEST_ASSERT_TRUE(expr) \
    TEST_ASSERT((expr))

/**
 * @brief Macro to assert that an expression is false
 * @param expr The expression to evaluate
 */
#define TEST_ASSERT_FALSE(expr) \
    TEST_ASSERT(!(expr))

/**
 * @brief Macro to assert that a is greater than b
 * @param a The first value
 * @param b The second value
 */
#define TEST_ASSERT_GT(a, b) \
    TEST_ASSERT((a) > (b))

/**
 * @brief Macro to assert that a is greater than or equal to b
 * @param a The first value
 * @param b The second value
 */
#define TEST_ASSERT_GE(a, b) \
    TEST_ASSERT((a) >= (b))

/**
 * @brief Macro to assert that a is less than b
 * @param a The first value
 * @param b The second value
 */
#define TEST_ASSERT_LT(a, b) \
    TEST_ASSERT((a) < (b))

/**
 * @brief Macro to assert that a is less than or equal to b
 * @param a The first value
 * @param b The second value
 */
#define TEST_ASSERT_LE(a, b) \
    TEST_ASSERT((a) <= (b))

/**
 * @brief Macro to assert that two strings are equal
 * @param a The first string
 * @param b The second string
 */
#define TEST_ASSERT_STR_EQ(a, b) \
    TEST_ASSERT(TEST_FRAMEWORK_STRCMP((a), (b)) == 0)

/**
 * @brief Macro to assert that two memory regions are equal
 * @param a The first memory region
 * @param b The second memory region
 * @param len The length of the memory regions to compare
 */
#define TEST_ASSERT_MEM_EQ(a, b, len) \
    TEST_ASSERT(TEST_FRAMEWORK_MEMCMP((a), (b), (len)) == 0)

/**
 * @brief Macro to define a test case function
 * @param name The name of the test case
 */
#define TEST_CASE(name) \
    static int test_##name(void)

/**
 * @brief Macro to define a test case function that is ignored/skipped
 * @param name The name of the test case
 */
#define TEST_CASE_IGNORE(name)                                  \
    MAYBE_UNUSED static int test_##name##_disabled(void);       \
    static int test_##name(void)                                \
    {                                                           \
        return TEST_SKIP;                                       \
    }                                                           \
    MAYBE_UNUSED static int test_##name##_disabled(void)

/**
 * @brief Macro to create a test registry entry for a test case
 * @param name The name of the test case
 */
#define TEST_ENTRY(name) \
    { #name, test_##name }

/**
 * @brief Macro to define the end of a test case array
 */
#define TEST_SUITE_END \
    { NULL, NULL }

/**
 * @brief Macro to define a test suite with a name and an array of test cases
 * @param suite_name The name of the test suite
 * @param case_array The array of test cases
 */
#define TEST_SUITE(suite_name, case_array)          \
{                                                   \
    .name = (suite_name),                           \
    .cases = (case_array),                          \
    .count = ARRAY_SIZE(case_array) - 1             \
}

#endif
