#include "test_framework.h"

static struct test_stats stats;

static void test_write(const char* str)
{
    user_print(str);
}

static void test_write_dec(const uint32_t value)
{
    user_print_dec((int)value);
}

static void test_set_color(const uint8_t fg, const uint8_t bg)
{
    UNUSED(fg);
    UNUSED(bg);
}

static void test_write_pass(void)
{
    test_set_color(0, 0);
    test_write("PASS");
}

static void test_write_fail(void)
{
    test_set_color(0, 0);
    test_write("FAIL");
}

static void test_write_skip(void)
{
    test_set_color(0, 0);
    test_write("SKIP");
}

void test_init(void)
{
    stats.total = 0;
    stats.passed = 0;
    stats.failed = 0;
    stats.skipped = 0;
}

void test_init_console(void)
{
    test_init();
}

int test_run_case(const char* name, const test_func_t func)
{
    stats.total++;
    user_print("  [");
    user_print(name);
    user_print("] ");

    const int result = func();
    if (result == TEST_PASS)
    {
        stats.passed++;
        test_write_pass();
        user_print("\n");
    }
    else if (result == TEST_FAIL)
    {
        stats.failed++;
        test_write_fail();
        user_print("\n");
    }
    else
    {
        stats.skipped++;
        test_write_skip();
        user_print("\n");
    }

    return result;
}

void test_run_suite(struct test_suite* suite)
{
    if (!suite || !suite->cases)
    {
        return;
    }

    user_print("\n=== ");
    user_print(suite->name);
    user_print(" ===\n");

    for (uint32_t i = 0; i < suite->count; i++)
    {
        if (suite->cases[i].name && suite->cases[i].func)
        {
            test_run_case(suite->cases[i].name, suite->cases[i].func);
        }
    }
}

struct test_stats* test_get_stats(void)
{
    return &stats;
}

void test_summary(void)
{
    user_print("\n=== Test Summary ===\n");
    user_print("Total:   ");
    test_write_dec(stats.total);
    user_print("\nPassed:  ");
    test_write_dec(stats.passed);
    user_print("\nFailed:  ");
    test_write_dec(stats.failed);
    user_print("\nSkipped: ");
    test_write_dec(stats.skipped);
    user_print("\n");

    if (stats.failed == 0)
    {
        user_print("\nAll tests passed!\n");
    }
    else
    {
        user_print("\nSome tests failed!\n");
    }
}

void test_assert_fail(const char* file, const int line, const char* expr)
{
    user_print("\n    ASSERTION FAILED: ");
    user_print(expr);
    user_print("\n    at ");
    user_print(file);
    user_print(":");
    test_write_dec((uint32_t)line);
    user_print("\n");
}

struct test_value test_make_value_u32(const uint32_t value)
{
    return (struct test_value){ .type = TEST_VALUE_U32, .u32 = value };
}

struct test_value test_make_value_u64(const uint64_t value)
{
    return (struct test_value){ .type = TEST_VALUE_U64, .u64 = value };
}

struct test_value test_make_value_s32(const int32_t value)
{
    return (struct test_value){ .type = TEST_VALUE_S32, .s32 = value };
}

struct test_value test_make_value_s64(const int64_t value)
{
    return (struct test_value){ .type = TEST_VALUE_S64, .s64 = value };
}

struct test_value test_make_value_f32(const float32_t value)
{
    return (struct test_value){ .type = TEST_VALUE_F32, .f32 = value };
}

struct test_value test_make_value_f64(const float64_t value)
{
    return (struct test_value){ .type = TEST_VALUE_F64, .f64 = value };
}

struct test_value test_make_value_ptr(const void* const value)
{
    return (struct test_value){ .type = TEST_VALUE_PTR, .ptr = (uintptr_t)value };
}

void test_assert_print_fail(const char* file, const int line, const bool equal, const char* actual_expr, const char* expected_expr, struct test_value actual, struct test_value expected)
{
    user_print("\n    ASSERTION FAILED\n");
    user_print("      Expression: ");
    user_print(actual_expr);
    user_print(equal ? " != " : " == ");
    user_print(expected_expr);
    user_print("\n      Location:   ");
    user_print(file);
    user_print(":");
    test_write_dec((uint32_t)line);
    user_print("\n");

    switch (actual.type)
    {
        case TEST_VALUE_U32:
            user_print("      Actual:     ");
            test_write_dec(actual.u32);
            user_print("\n      Expected:   ");
            test_write_dec(expected.u32);
            user_print("\n");
            break;
        case TEST_VALUE_U64:
            user_print("      Actual:     ");
            user_print_dec((int)actual.u64);
            user_print("\n      Expected:   ");
            user_print_dec((int)expected.u64);
            user_print("\n");
            break;
        case TEST_VALUE_S32:
            user_print("      Actual:     ");
            user_print_dec(actual.s32);
            user_print("\n      Expected:   ");
            user_print_dec(expected.s32);
            user_print("\n");
            break;
        case TEST_VALUE_S64:
            user_print("      Actual:     ");
            user_print_dec((int)actual.s64);
            user_print("\n      Expected:   ");
            user_print_dec((int)expected.s64);
            user_print("\n");
            break;
        case TEST_VALUE_F32:
        case TEST_VALUE_F64:
        case TEST_VALUE_PTR:
        default:
            user_print("      Actual/Expected reporting not implemented for this value type\n");
            break;
    }
}
