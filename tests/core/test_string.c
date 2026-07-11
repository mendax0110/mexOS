#include "test_string.h"
#include "../lib/string.h"

TEST_CASE(string_strlen_empty)
{
    TEST_ASSERT_EQ(strlen(""), 0);
    return TEST_PASS;
}

TEST_CASE(string_strlen_normal)
{
    TEST_ASSERT_EQ(strlen("hello"), 5);
    return TEST_PASS;
}

TEST_CASE(string_strlen_long)
{
    TEST_ASSERT_EQ(strlen("the quick brown fox"), 19);
    return TEST_PASS;
}

TEST_CASE(string_strcmp_equal)
{
    TEST_ASSERT_EQ(strcmp("abc", "abc"), 0);
    return TEST_PASS;
}

TEST_CASE(string_strcmp_less)
{
    TEST_ASSERT_LT(strcmp("abc", "abd"), 0);
    return TEST_PASS;
}

TEST_CASE(string_strcmp_greater)
{
    TEST_ASSERT_GT(strcmp("abd", "abc"), 0);
    return TEST_PASS;
}

TEST_CASE(string_strcmp_empty)
{
    TEST_ASSERT_EQ(strcmp("", ""), 0);
    return TEST_PASS;
}

TEST_CASE(string_strcmp_length_diff)
{
    TEST_ASSERT_LT(strcmp("ab", "abc"), 0);
    TEST_ASSERT_GT(strcmp("abc", "ab"), 0);
    return TEST_PASS;
}

TEST_CASE(string_strncmp_equal)
{
    TEST_ASSERT_EQ(strncmp("abcdef", "abcxyz", 3), 0);
    return TEST_PASS;
}

TEST_CASE(string_strncmp_less)
{
    TEST_ASSERT_LT(strncmp("abc", "abd", 3), 0);
    return TEST_PASS;
}

TEST_CASE(string_strcpy_normal)
{
    char buf[32];
    strcpy(buf, "hello");
    TEST_ASSERT_STR_EQ(buf, "hello");
    return TEST_PASS;
}

TEST_CASE(string_strcpy_empty)
{
    char buf[32] = "garbage";
    strcpy(buf, "");
    TEST_ASSERT_STR_EQ(buf, "");
    return TEST_PASS;
}

TEST_CASE(string_strncpy_normal)
{
    char buf[32];
    memset(buf, 'X', sizeof(buf));
    strncpy(buf, "hello", 5);
    TEST_ASSERT_EQ(memcmp(buf, "hello", 5), 0);
    return TEST_PASS;
}

TEST_CASE(string_strncpy_truncate)
{
    char buf[4];
    strncpy(buf, "hello", 3);
    TEST_ASSERT_EQ(memcmp(buf, "hel", 3), 0);
    return TEST_PASS;
}

TEST_CASE(string_strcat_normal)
{
    char buf[32] = "hello";
    strcat(buf, " world");
    TEST_ASSERT_STR_EQ(buf, "hello world");
    return TEST_PASS;
}

TEST_CASE(string_strcat_empty)
{
    char buf[32] = "hello";
    strcat(buf, "");
    TEST_ASSERT_STR_EQ(buf, "hello");
    return TEST_PASS;
}

TEST_CASE(string_memset_normal)
{
    char buf[16];
    memset(buf, 'A', 16);
    int match = 1;
    for (int i = 0; i < 16; i++)
    {
        if (buf[i] != 'A')
        {
            match = 0;
            break;
        }
    }
    TEST_ASSERT(match);
    return TEST_PASS;
}

TEST_CASE(string_memset_zero)
{
    char buf[16] = "garbage";
    memset(buf, 0, 16);
    int match = 1;
    for (int i = 0; i < 16; i++)
    {
        if (buf[i] != 0)
        {
            match = 0;
            break;
        }
    }
    TEST_ASSERT(match);
    return TEST_PASS;
}

TEST_CASE(string_memcpy_normal)
{
    const char src[] = "hello world";
    char dst[16];
    memcpy(dst, src, 12);
    TEST_ASSERT_STR_EQ(dst, src);
    return TEST_PASS;
}

TEST_CASE(string_memcpy_partial)
{
    const char src[] = "abcdefgh";
    char dst[16];
    memset(dst, 0, 16);
    memcpy(dst, src, 4);
    TEST_ASSERT_EQ(memcmp(dst, "abcd", 4), 0);
    return TEST_PASS;
}

TEST_CASE(string_memcmp_equal)
{
    const char a[] = "test";
    const char b[] = "test";
    TEST_ASSERT_EQ(memcmp(a, b, 4), 0);
    return TEST_PASS;
}

TEST_CASE(string_memcmp_diff)
{
    const char a[] = "test";
    const char b[] = "tesx";
    TEST_ASSERT_NEQ(memcmp(a, b, 4), 0);
    return TEST_PASS;
}

TEST_CASE(string_memmov_normal)
{
    char buf[16] = "abcdefgh";
    memmov(buf + 2, buf, 6);
    TEST_ASSERT_EQ(memcmp(buf, "ababcdef", 8), 0);
    return TEST_PASS;
}

TEST_CASE(string_strncat_normal)
{
    char buf[32];
    memset(buf, 0, 32);
    strncat(buf, "hello", 5);
    TEST_ASSERT_EQ(memcmp(buf, "hello", 5), 0);
    return TEST_PASS;
}

TEST_CASE(string_snprintf_normal)
{
    char buf[32];

    const struct { const char* fmt; const char* expected; } cases[] = {
        { "zero pad: %05d, width: %5d", "zero pad: 00007, width:     7" },
        { "negative: %d, zero: %d",     "negative: -42, zero: 0" },
        { "long string: %.10s",         "long string: this is a " },
        { "char: %c, string: %s",       "char: A, string: test" },
        { "hex with width: %08x",       "hex with width: 001a2b3c" },
    };

    snprintf(buf, sizeof(buf), cases[0].fmt, 7, 7);
    TEST_ASSERT_STR_EQ(buf, cases[0].expected);

    snprintf(buf, sizeof(buf), cases[1].fmt, -42, 0);
    TEST_ASSERT_STR_EQ(buf, cases[1].expected);

    snprintf(buf, sizeof(buf), cases[2].fmt, "this is a long string");
    TEST_ASSERT_STR_EQ(buf, cases[2].expected);

    snprintf(buf, sizeof(buf), cases[3].fmt, 'A', "test");
    TEST_ASSERT_STR_EQ(buf, cases[3].expected);

    snprintf(buf, sizeof(buf), cases[4].fmt, 0x1A2B3C);
    TEST_ASSERT_STR_EQ(buf, cases[4].expected);

    return TEST_PASS;
}

static struct test_case string_cases[] = {
        TEST_ENTRY(string_strlen_empty),
        TEST_ENTRY(string_strlen_normal),
        TEST_ENTRY(string_strlen_long),
        TEST_ENTRY(string_strcmp_equal),
        TEST_ENTRY(string_strcmp_less),
        TEST_ENTRY(string_strcmp_greater),
        TEST_ENTRY(string_strcmp_empty),
        TEST_ENTRY(string_strcmp_length_diff),
        TEST_ENTRY(string_strncmp_equal),
        TEST_ENTRY(string_strncmp_less),
        TEST_ENTRY(string_strcpy_normal),
        TEST_ENTRY(string_strcpy_empty),
        TEST_ENTRY(string_strncpy_normal),
        TEST_ENTRY(string_strncpy_truncate),
        TEST_ENTRY(string_strcat_normal),
        TEST_ENTRY(string_strcat_empty),
        TEST_ENTRY(string_memset_normal),
        TEST_ENTRY(string_memset_zero),
        TEST_ENTRY(string_memcpy_normal),
        TEST_ENTRY(string_memcpy_partial),
        TEST_ENTRY(string_memcmp_equal),
        TEST_ENTRY(string_memcmp_diff),
        TEST_ENTRY(string_memmov_normal),
        TEST_ENTRY(string_strncat_normal),
        TEST_ENTRY(string_snprintf_normal),
        TEST_SUITE_END
};

static struct test_suite string_suite = TEST_SUITE("String Tests", string_cases);

struct test_suite* test_string_get_suite(void)
{
    return &string_suite;
}
