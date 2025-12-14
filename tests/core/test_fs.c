#include "test_fs.h"
#include "../../servers/vfs/fs.h"
#include "../../shared/string.h"

TEST_CASE(fs_create_file_success)
{
    const int ret = fs_create_file("/test_file_1.txt");
    TEST_ASSERT_EQ(ret, FS_ERR_OK);
    fs_remove("/test_file_1.txt");
    return TEST_PASS;
}

TEST_CASE(fs_create_file_exists)
{
    fs_create_file("/test_file_dup.txt");
    const int ret = fs_create_file("/test_file_dup.txt");
    TEST_ASSERT_EQ(ret, FS_ERR_EXISTS);
    fs_remove("/test_file_dup.txt");
    return TEST_PASS;
}

TEST_CASE(fs_create_dir_success)
{
    const int ret = fs_create_dir("/test_dir_1");
    TEST_ASSERT_EQ(ret, FS_ERR_OK);
    fs_remove("/test_dir_1");
    return TEST_PASS;
}

TEST_CASE(fs_create_dir_exists)
{
    fs_create_dir("/test_dir_dup");
    const int ret = fs_create_dir("/test_dir_dup");
    TEST_ASSERT_EQ(ret, FS_ERR_EXISTS);
    fs_remove("/test_dir_dup");
    return TEST_PASS;
}

TEST_CASE(fs_remove_file_success)
{
    fs_create_file("/test_rm_file.txt");
    const int ret = fs_remove("/test_rm_file.txt");
    TEST_ASSERT_EQ(ret, FS_ERR_OK);
    return TEST_PASS;
}

TEST_CASE(fs_remove_not_found)
{
    const int ret = fs_remove("/nonexistent_file_xyz.txt");
    TEST_ASSERT_EQ(ret, FS_ERR_NOT_FOUND);
    return TEST_PASS;
}

TEST_CASE(fs_remove_empty_dir)
{
    fs_create_dir("/test_rm_dir");
    const int ret = fs_remove("/test_rm_dir");
    TEST_ASSERT_EQ(ret, FS_ERR_OK);
    return TEST_PASS;
}

TEST_CASE(fs_write_read_roundtrip)
{
    fs_create_file("/test_rw.txt");
    const char* data = "hello world";
    const int write_ret = fs_write("/test_rw.txt", data, 11);
    TEST_ASSERT_GE(write_ret, 0);
    char buf[64];
    memset(buf, 0, sizeof(buf));
    const int read_ret = fs_read("/test_rw.txt", buf, sizeof(buf));
    TEST_ASSERT_GT(read_ret, 0);
    TEST_ASSERT_STR_EQ(buf, data);
    fs_remove("/test_rw.txt");
    return TEST_PASS;
}

TEST_CASE(fs_read_nonexistent)
{
    char buf[64];
    const int ret = fs_read("/nonexistent_read.txt", buf, sizeof(buf));
    TEST_ASSERT_EQ(ret, FS_ERR_NOT_FOUND);
    return TEST_PASS;
}

TEST_CASE(fs_write_nonexistent)
{
    const int ret = fs_write("/nonexistent_write.txt", "data", 4);
    TEST_ASSERT_EQ(ret, FS_ERR_NOT_FOUND);
    return TEST_PASS;
}

TEST_CASE(fs_exists_true)
{
    fs_create_file("/test_exists.txt");
    const int ret = fs_exists("/test_exists.txt");
    TEST_ASSERT_EQ(ret, 1);
    fs_remove("/test_exists.txt");
    return TEST_PASS;
}

TEST_CASE(fs_exists_false)
{
    const int ret = fs_exists("/definitely_not_exists.txt");
    TEST_ASSERT_EQ(ret, 0);
    return TEST_PASS;
}

TEST_CASE(fs_is_dir_true)
{
    fs_create_dir("/test_isdir");
    const int ret = fs_is_dir("/test_isdir");
    TEST_ASSERT_EQ(ret, 1);
    fs_remove("/test_isdir");
    return TEST_PASS;
}

TEST_CASE(fs_is_dir_false)
{
    fs_create_file("/test_notdir.txt");
    const int ret = fs_is_dir("/test_notdir.txt");
    TEST_ASSERT_EQ(ret, 0);
    fs_remove("/test_notdir.txt");
    return TEST_PASS;
}

TEST_CASE(fs_get_size_empty)
{
    fs_create_file("/test_size_empty.txt");
    const uint32_t size = fs_get_size("/test_size_empty.txt");
    TEST_ASSERT_EQ(size, 0);
    fs_remove("/test_size_empty.txt");
    return TEST_PASS;
}

TEST_CASE(fs_get_size_with_data)
{
    fs_create_file("/test_size_data.txt");
    fs_write("/test_size_data.txt", "1234567890", 10);
    const uint32_t size = fs_get_size("/test_size_data.txt");
    TEST_ASSERT_EQ(size, 10);
    fs_remove("/test_size_data.txt");
    return TEST_PASS;
}

TEST_CASE(fs_append_data)
{
    fs_create_file("/test_append.txt");
    fs_write("/test_append.txt", "hello", 5);
    fs_append("/test_append.txt", " world", 6);
    char buf[64];
    memset(buf, 0, sizeof(buf));
    fs_read("/test_append.txt", buf, sizeof(buf));
    TEST_ASSERT_STR_EQ(buf, "hello world");
    fs_remove("/test_append.txt");
    return TEST_PASS;
}

TEST_CASE(fs_nested_dir)
{
    fs_create_dir("/test_nest");
    const int ret = fs_create_file("/test_nest/file.txt");
    TEST_ASSERT_EQ(ret, FS_ERR_OK);
    fs_remove("/test_nest/file.txt");
    fs_remove("/test_nest");
    return TEST_PASS;
}

TEST_CASE(fs_cwd_not_null)
{
    const char* cwd = fs_get_cwd();
    TEST_ASSERT_NOT_NULL(cwd);
    return TEST_PASS;
}

static struct test_case fs_cases[] = {
        TEST_ENTRY(fs_create_file_success),
        TEST_ENTRY(fs_create_file_exists),
        TEST_ENTRY(fs_create_dir_success),
        TEST_ENTRY(fs_create_dir_exists),
        TEST_ENTRY(fs_remove_file_success),
        TEST_ENTRY(fs_remove_not_found),
        TEST_ENTRY(fs_remove_empty_dir),
        TEST_ENTRY(fs_write_read_roundtrip),
        TEST_ENTRY(fs_read_nonexistent),
        TEST_ENTRY(fs_write_nonexistent),
        TEST_ENTRY(fs_exists_true),
        TEST_ENTRY(fs_exists_false),
        TEST_ENTRY(fs_is_dir_true),
        TEST_ENTRY(fs_is_dir_false),
        TEST_ENTRY(fs_get_size_empty),
        TEST_ENTRY(fs_get_size_with_data),
        TEST_ENTRY(fs_append_data),
        TEST_ENTRY(fs_nested_dir),
        TEST_ENTRY(fs_cwd_not_null),
        TEST_SUITE_END
};

static struct test_suite fs_suite = {
        .name = "Filesystem Tests",
        .cases = fs_cases,
        .count = 19
};

struct test_suite* test_fs_get_suite(void)
{
    return &fs_suite;
}
