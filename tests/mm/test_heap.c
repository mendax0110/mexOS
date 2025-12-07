#include "test_heap.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/include/string.h"

TEST_CASE(heap_kmalloc_returns_non_null)
{
    void* ptr = kmalloc(64);
    TEST_ASSERT_NOT_NULL(ptr);
    kfree(ptr);
    return TEST_PASS;
}

TEST_CASE(heap_kmalloc_small_alloc)
{
    void* ptr = kmalloc(1);
    TEST_ASSERT_NOT_NULL(ptr);
    kfree(ptr);
    return TEST_PASS;
}

TEST_CASE(heap_kmalloc_medium_alloc)
{
    void* ptr = kmalloc(256);
    TEST_ASSERT_NOT_NULL(ptr);
    kfree(ptr);
    return TEST_PASS;
}

TEST_CASE(heap_kmalloc_large_alloc)
{
    void* ptr = kmalloc(4096);
    TEST_ASSERT_NOT_NULL(ptr);
    kfree(ptr);
    return TEST_PASS;
}

TEST_CASE(heap_kmalloc_multiple_unique)
{
    void* p1 = kmalloc(32);
    void* p2 = kmalloc(32);
    void* p3 = kmalloc(32);
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);
    TEST_ASSERT_NEQ(p1, p2);
    TEST_ASSERT_NEQ(p2, p3);
    TEST_ASSERT_NEQ(p1, p3);
    kfree(p1);
    kfree(p2);
    kfree(p3);
    return TEST_PASS;
}

TEST_CASE(heap_kfree_null_safe)
{
    kfree(NULL);
    return TEST_PASS;
}

TEST_CASE(heap_kmalloc_aligned_16)
{
    void* ptr = kmalloc_aligned(64, 16);
    if (ptr == NULL)
    {
        return TEST_SKIP;
    }
    const uint32_t addr = (uint32_t)ptr;
    const int aligned = (addr % 16) == 0;
    kfree(ptr);
    TEST_ASSERT(aligned);
    return TEST_PASS;
}

TEST_CASE(heap_kmalloc_aligned_4096)
{
    void* ptr = kmalloc_aligned(4096, 4096);
    if (ptr == NULL)
    {
        return TEST_SKIP;
    }
    const uint32_t addr = (uint32_t)ptr;
    const int aligned = (addr % 4096) == 0;
    kfree(ptr);
    TEST_ASSERT(aligned);
    return TEST_PASS;
}

TEST_CASE(heap_alloc_write_read)
{
    char* ptr = (char*)kmalloc(128);
    TEST_ASSERT_NOT_NULL(ptr);
    memset(ptr, 0xAB, 128);
    int match = 1;
    for (int i = 0; i < 128; i++)
    {
        if ((uint8_t)ptr[i] != 0xAB)
        {
            match = 0;
            break;
        }
    }
    kfree(ptr);
    TEST_ASSERT(match);
    return TEST_PASS;
}

TEST_CASE(heap_reuse_after_free)
{
    void* p1 = kmalloc(64);
    TEST_ASSERT_NOT_NULL(p1);
    kfree(p1);
    void* p2 = kmalloc(64);
    TEST_ASSERT_NOT_NULL(p2);
    kfree(p2);
    return TEST_PASS;
}

TEST_CASE(heap_stats_positive)
{
    const size_t used = heap_get_used();
    const size_t free = heap_get_free();
    (void)used;
    TEST_ASSERT_GT(free, 0);
    return TEST_PASS;
}

TEST_CASE(heap_stats_change_on_alloc)
{
    const size_t free_before = heap_get_free();
    void* ptr = kmalloc(1024);
    if (ptr == NULL)
    {
        return TEST_SKIP;
    }
    const size_t free_after = heap_get_free();
    TEST_ASSERT_LT(free_after, free_before);
    kfree(ptr);
    return TEST_PASS;
}

static struct test_case heap_cases[] = {
        TEST_ENTRY(heap_kmalloc_returns_non_null),
        TEST_ENTRY(heap_kmalloc_small_alloc),
        TEST_ENTRY(heap_kmalloc_medium_alloc),
        TEST_ENTRY(heap_kmalloc_large_alloc),
        TEST_ENTRY(heap_kmalloc_multiple_unique),
        TEST_ENTRY(heap_kfree_null_safe),
        TEST_ENTRY(heap_kmalloc_aligned_16),
        TEST_ENTRY(heap_kmalloc_aligned_4096),
        TEST_ENTRY(heap_alloc_write_read),
        TEST_ENTRY(heap_reuse_after_free),
        TEST_ENTRY(heap_stats_positive),
        TEST_ENTRY(heap_stats_change_on_alloc),
        TEST_SUITE_END
};

static struct test_suite heap_suite = {
        .name = "Heap Tests",
        .cases = heap_cases,
        .count = 12
};

struct test_suite* test_heap_get_suite(void)
{
    return &heap_suite;
}
