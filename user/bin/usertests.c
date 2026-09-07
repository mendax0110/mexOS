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

TEST_CASE(parse_int_handles_invalid)
{
    TEST_ASSERT_EQ(parse_int("abc"), 0);
    TEST_ASSERT_EQ(parse_int("123abc"), 123);
    return TEST_PASS;
}

TEST_CASE(parse_int_handles_overflow)
{
    TEST_ASSERT_EQ(parse_int("99999999999999999999"), INT32_MAX);
    return TEST_PASS;
}

TEST_CASE(parse_int_handles_negative_overflow)
{
    TEST_ASSERT_EQ(parse_int("-99999999999999999999"), INT32_MIN);
    return TEST_PASS;
}

TEST_CASE(parse_int_handles_empty_string)
{
    TEST_ASSERT_EQ(parse_int(""), 0);
    return TEST_PASS;
}

TEST_CASE(from_hex_converts)
{
    TEST_ASSERT_EQ(from_hex("1A", 2), 26);
    TEST_ASSERT_EQ(from_hex("FF", 2), 255);
    TEST_ASSERT_EQ(from_hex("0", 1), 0);
    return TEST_PASS;
}

TEST_CASE(to_hex_converts)
{
    uint32_t result = to_hex('A');
    TEST_ASSERT_EQ(result, 10);
    result = to_hex('f');
    TEST_ASSERT_EQ(result, 15);
    result = to_hex('0');
    TEST_ASSERT_EQ(result, 0);
    result = to_hex('G');
    TEST_ASSERT_EQ(result, UINT32_INVALID);
    return TEST_PASS;
}

TEST_CASE(append_uint_dec_works)
{
    char buf[16];
    copy_string(buf, sizeof(buf), "Value: ");
    append_uint_dec(buf, sizeof(buf), 12345);
    TEST_ASSERT_STR_EQ(buf, "Value: 12345");
    return TEST_PASS;
}

TEST_CASE(append_int_dec_works)
{
    char buf[16];
    copy_string(buf, sizeof(buf), "Value: ");
    append_int_dec(buf, sizeof(buf), -6789);
    TEST_ASSERT_STR_EQ(buf, "Value: -6789");
    return TEST_PASS;
}

static struct test_case helper_cases[] =
{
    TEST_ENTRY(copy_string_copies),
    TEST_ENTRY(append_string_appends),
    TEST_ENTRY(path_basename_extracts),
    TEST_ENTRY(parse_int_handles_negative_overflow),
    TEST_ENTRY(parse_int_handles_overflow),
    TEST_ENTRY(parse_int_handles_invalid),
    TEST_ENTRY(parse_int_handles_sign),
    TEST_ENTRY(parse_int_handles_empty_string),
    TEST_ENTRY(from_hex_converts),
    TEST_ENTRY(to_hex_converts),
    TEST_ENTRY(append_uint_dec_works),
    TEST_ENTRY(append_int_dec_works),
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
