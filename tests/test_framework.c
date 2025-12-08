#include "test_framework.h"
#include "../kernel/ui/vterm.h"
#include "../kernel/ui/console.h"
#include "../kernel/include/string.h"

static struct test_stats stats;
static struct vterm* test_vterm = NULL;

static void test_write(const char* str)
{
    if (test_vterm)
    {
        vterm_write(test_vterm, str);
    }
}

static void test_write_dec(uint32_t val)
{
    if (test_vterm)
    {
        vterm_write_dec(test_vterm, val);
    }
}

static void test_set_color(uint8_t fg, uint8_t bg)
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

int test_run_case(const char* name, test_func_t func)
{
    int result;

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

    result = func();

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
    uint32_t i;

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

    for (i = 0; i < suite->count; i++)
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

void test_assert_fail(const char* file, int line, const char* expr)
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
