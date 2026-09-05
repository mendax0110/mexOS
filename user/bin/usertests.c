#include "../lib/tests/test_runner.h"
#include "../shared/string_utils.h"

#define TEST_FRAMEWORK_STRCMP user_strcmp
#define TEST_FRAMEWORK_MEMCMP user_memcmp

TEST_CASE(copy_string_copies)
{
    char buf[16];
    copy_string(buf, sizeof(buf), "hello");
    TEST_ASSERT_STR_EQ(buf, "hello");
    return TEST_PASS;
}

TEST_CASE(append_string_appends)
{
    char buf[16];
    copy_string(buf, sizeof(buf), "hi");
    append_string(buf, sizeof(buf), "!");
    TEST_ASSERT_STR_EQ(buf, "hi!");
    return TEST_PASS;
}

TEST_CASE(path_basename_extracts)
{
    TEST_ASSERT_STR_EQ(path_basename("/bin/sh"), "sh");
    TEST_ASSERT_STR_EQ(path_basename("ls"), "ls");
    return TEST_PASS;
}

TEST_CASE(parse_int_handles_sign)
{
    TEST_ASSERT_EQ(parse_int("-42"), -42);
    TEST_ASSERT_EQ(parse_int("17"), 17);
    return TEST_PASS;
}

static struct test_case helper_cases[] =
{
    TEST_ENTRY(copy_string_copies),
    TEST_ENTRY(append_string_appends),
    TEST_ENTRY(path_basename_extracts),
    TEST_ENTRY(parse_int_handles_sign),
    TEST_SUITE_END
};

static struct test_suite helper_suite = TEST_SUITE("Userspace Helper Tests", helper_cases);

struct test_suite* test_user_helpers_get_suite(void)
{
    return &helper_suite;
}

int main(void)
{
    run_all_tests_console();
    return 0;
}
