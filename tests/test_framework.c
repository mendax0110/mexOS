#include "test_framework.h"
#include "../kernel/ui/vterm.h"
#include "../kernel/ui/console.h"

static struct test_stats stats;
static struct vterm* test_vterm = NULL;

static void test_write(const char* str)
{
    if (test_vterm)
    {
        vterm_write(test_vterm, str);
    }
}

#define test_write_dec(val)                                 \
    do                                                      \
    {                                                       \
        if (test_vterm)                                     \
        {                                                   \
            vterm_write_dec(test_vterm, (val));             \
        }                                                   \
    } while (0)

#define PRINT_VALUE(val)                            \
    do                                              \
    {                                               \
        if (test_vterm)                             \
        {                                           \
            test_write("      Actual:     ");       \
            test_write_dec(actual.val);             \
            test_write("\n");                       \
            test_write("      Expected:   ");       \
            test_write_dec(expected.val);           \
            test_write("\n");                       \
        }                                           \
        else                                        \
        {                                           \
            console_write("      Actual:     ");    \
            console_write_dec(actual.val);          \
            console_write("\n");                    \
            console_write("      Expected:   ");    \
            console_write_dec(expected.val);        \
            console_write("\n");                    \
        }                                           \
    } while (0)

static void test_set_color(const uint8_t fg, const uint8_t bg)
{
    if (test_vterm)
    {
        vterm_set_color(test_vterm, fg, bg);
    }
}

static void test_write_pass(void)
{
    test_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    test_write("PASS");
    test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

static void test_write_fail(void)
{
    test_set_color(VGA_LIGHT_RED, VGA_BLACK);
    test_write("FAIL");
    test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

static void test_write_skip(void)
{
    test_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
    test_write("SKIP");
    test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

void test_init(void)
{
    stats.total = 0;
    stats.passed = 0;
    stats.failed = 0;
    stats.skipped = 0;
    test_vterm = vterm_get(VTERM_USER1);
}

void test_init_console(void)
{
    stats.total = 0;
    stats.passed = 0;
    stats.failed = 0;
    stats.skipped = 0;
    test_vterm = NULL;
}

int test_run_case(const char* name, const test_func_t func)
{
    stats.total++;

    if (test_vterm)
    {
        test_write("  [");
        test_write(name);
        test_write("] ");
    }
    else
    {
        console_write("  [");
        console_write(name);
        console_write("] ");
    }

    const int result = func();

    if (result == TEST_PASS)
    {
        stats.passed++;
        if (test_vterm)
        {
            test_write_pass();
            test_write("\n");
        }
        else
        {
            console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            console_write("PASS");
            console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            console_write("\n");
        }
    }
    else if (result == TEST_FAIL)
    {
        stats.failed++;
        if (test_vterm)
        {
            test_write_fail();
            test_write("\n");
        }
        else
        {
            console_set_color(VGA_LIGHT_RED, VGA_BLACK);
            console_write("FAIL");
            console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            console_write("\n");
        }
    }
    else
    {
        stats.skipped++;
        if (test_vterm)
        {
            test_write_skip();
            test_write("\n");
        }
        else
        {
            console_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
            console_write("SKIP");
            console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            console_write("\n");
        }
    }

    return result;
}

void test_run_suite(struct test_suite* suite)
{
    if (!suite || !suite->cases)
    {
        return;
    }

    if (test_vterm)
    {
        test_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        test_write("\n=== ");
        test_write(suite->name);
        test_write(" ===\n");
        test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
    else
    {
        console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        console_write("\n=== ");
        console_write(suite->name);
        console_write(" ===\n");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }

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
    if (test_vterm)
    {
        test_set_color(VGA_WHITE, VGA_BLACK);
        test_write("\n=== Test Summary ===\n");
        test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        test_write("Total:   ");
        test_write_dec(stats.total);
        test_write("\n");
        test_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        test_write("Passed:  ");
        test_write_dec(stats.passed);
        test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        test_write("\n");
        test_set_color(VGA_LIGHT_RED, VGA_BLACK);
        test_write("Failed:  ");
        test_write_dec(stats.failed);
        test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        test_write("\n");
        test_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
        test_write("Skipped: ");
        test_write_dec(stats.skipped);
        test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        test_write("\n");

        if (stats.failed == 0)
        {
            test_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            test_write("\nAll tests passed!\n");
        }
        else
        {
            test_set_color(VGA_LIGHT_RED, VGA_BLACK);
            test_write("\nSome tests failed!\n");

        }
        test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
    else
    {
        console_set_color(VGA_WHITE, VGA_BLACK);
        console_write("\n=== Test Summary ===\n");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("Total:   ");
        console_write_dec(stats.total);
        console_write("\n");
        console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        console_write("Passed:  ");
        console_write_dec(stats.passed);
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("\n");
        console_set_color(VGA_LIGHT_RED, VGA_BLACK);
        console_write("Failed:  ");
        console_write_dec(stats.failed);
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("\n");
        console_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
        console_write("Skipped: ");
        console_write_dec(stats.skipped);
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("\n");

        if (stats.failed == 0)
        {
            console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            console_write("\nAll tests passed!\n");
        }
        else
        {
            console_set_color(VGA_LIGHT_RED, VGA_BLACK);
            console_write("\nSome tests failed!\n");
        }
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

void test_assert_fail(const char* file, const int line, const char* expr)
{
    if (test_vterm)
    {
        test_set_color(VGA_LIGHT_RED, VGA_BLACK);
        test_write("\n    ASSERTION FAILED: ");
        test_write(expr);
        test_write("\n    at ");
        test_write(file);
        test_write(":");
        test_write_dec(line);
        test_write("\n");
        test_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
    else
    {
        console_set_color(VGA_LIGHT_RED, VGA_BLACK);
        console_write("\n    ASSERTION FAILED: ");
        console_write(expr);
        console_write("\n    at ");
        console_write(file);
        console_write(":");
        console_write_dec(line);
        console_write("\n");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

struct test_value test_make_value_u32(const uint32_t value)
{
    return (struct test_value)
    {
        .type = TEST_VALUE_U32,
        .u32 = value
    };
}

struct test_value test_make_value_u64(const uint64_t value)
{
    return (struct test_value)
    {
        .type = TEST_VALUE_U64,
        .u64 = value
    };
}

struct test_value test_make_value_s32(const int32_t value)
{
    return (struct test_value)
    {
        .type = TEST_VALUE_S32,
        .s32 = value
    };
}

struct test_value test_make_value_s64(const int64_t value)
{
    return (struct test_value)
    {
        .type = TEST_VALUE_S64,
        .s64 = value
    };
}

struct test_value test_make_value_f32(const float32_t value)
{
    return (struct test_value)
    {
        .type = TEST_VALUE_F32,
        .f32 = value
    };
}

struct test_value test_make_value_f64(const float64_t value)
{
    return (struct test_value)
    {
        .type = TEST_VALUE_F64,
        .f64 = value
    };
}

struct test_value test_make_value_ptr(const void* const value)
{
    return (struct test_value)
    {
        .type = TEST_VALUE_PTR,
        .ptr = (uintptr_t)value
    };
}


void test_assert_print_fail(const char* file, const int line, const bool equal, const char* actual_expr, const char* expected_expr, struct test_value actual, struct test_value expected)
{
    if (test_vterm)
    {
        test_set_color(VGA_LIGHT_RED, VGA_BLACK);

        test_write("\n    ASSERTION FAILED\n");

        test_set_color(VGA_LIGHT_GREY, VGA_BLACK);

        test_write("      Expression: ");
        test_write(actual_expr);
        if (equal)
        {
            test_write(" != ");
        }
        else
        {
            test_write(" == ");
        }
        test_write(expected_expr);

        test_write("\n      Location:   ");
        test_write(file);
        test_write(":");
        test_write_dec((uint32_t)line);

        test_write("\n");
    }
    else
    {
        console_set_color(VGA_LIGHT_RED, VGA_BLACK);

        console_write("\n    ASSERTION FAILED\n");

        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);

        console_write("      Expression: ");
        console_write(actual_expr);
        if (equal)
        {
            console_write(" != ");
        }
        else
        {
            console_write(" == ");
        }
        console_write(expected_expr);

        console_write("\n      Location:   ");
        console_write(file);
        console_write(":");
        console_write_dec((uint32_t)line);

        console_write("\n");
    }

    switch (actual.type)
    {
        case TEST_VALUE_U32:
            PRINT_VALUE(u32);
            break;
        case TEST_VALUE_U64:
            PRINT_VALUE(u64);
            break;
        case TEST_VALUE_S32:
            PRINT_VALUE(s32);
            break;
        case TEST_VALUE_S64:
            PRINT_VALUE(s64);
            break;
        case TEST_VALUE_F32:
            PRINT_VALUE(f32);
            break;
        case TEST_VALUE_F64:
            PRINT_VALUE(f64);
            break;
        case TEST_VALUE_PTR:
        default:
            PRINT_VALUE(ptr);
            break;
    }
}