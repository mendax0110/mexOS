#include "test_string.h"
#include "../../kernel/include/string.h"

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
    char src[] = "hello world";
    char dst[16];
    memcpy(dst, src, 12);
    TEST_ASSERT_STR_EQ(dst, src);
    return TEST_PASS;
}

TEST_CASE(string_memcpy_partial)
{
    char src[] = "abcdefgh";
    char dst[16];
    memset(dst, 0, 16);
    memcpy(dst, src, 4);
    TEST_ASSERT_EQ(memcmp(dst, "abcd", 4), 0);
    return TEST_PASS;
}

TEST_CASE(string_memcmp_equal)
{
    char a[] = "test";
    char b[] = "test";
    TEST_ASSERT_EQ(memcmp(a, b, 4), 0);
    return TEST_PASS;
}

TEST_CASE(string_memcmp_diff)
{
    char a[] = "test";
    char b[] = "tesx";
    TEST_ASSERT_NEQ(memcmp(a, b, 4), 0);
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
        TEST_SUITE_END
};

static struct test_suite string_suite = {
        .name = "String Tests",
        .cases = string_cases,
        .count = 22
};

struct test_suite* test_string_get_suite(void)
{
    return &string_suite;
}
