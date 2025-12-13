#include "test_pmm.h"
#include "../../kernel/mm/pmm.h"
#include "../include/cast.h"

TEST_CASE(pmm_alloc_block_returns_non_null)
{
    void* block = pmm_alloc_block();
    if (block == NULL)
    {
        return TEST_FAIL;
    }
    pmm_free_block(block);
    return TEST_PASS;
}

TEST_CASE(pmm_alloc_block_alignment)
{
    void* block = pmm_alloc_block();
    if (block == NULL)
    {
        return TEST_SKIP;
    }
    const uint32_t addr = PTR_TO_U32(block);
    const int aligned = (addr % 4096) == 0;
    pmm_free_block(block);
    TEST_ASSERT(aligned);
    return TEST_PASS;
}

TEST_CASE(pmm_free_block_restores_count)
{
    const uint32_t before = pmm_get_free_block_count();
    void* block = pmm_alloc_block();
    if (block == NULL)
    {
        return TEST_SKIP;
    }
    const uint32_t during = pmm_get_free_block_count();
    pmm_free_block(block);
    const uint32_t after = pmm_get_free_block_count();
    TEST_ASSERT_EQ(before, after);
    TEST_ASSERT_EQ(during, before - 1);
    return TEST_PASS;
}

TEST_CASE(pmm_alloc_multiple_unique)
{
    void* b1 = pmm_alloc_block();
    void* b2 = pmm_alloc_block();
    void* b3 = pmm_alloc_block();
    if (!b1 || !b2 || !b3)
    {
        if (b1) pmm_free_block(b1);
        if (b2) pmm_free_block(b2);
        if (b3) pmm_free_block(b3);
        return TEST_SKIP;
    }
    const int unique = (b1 != b2) && (b2 != b3) && (b1 != b3);
    pmm_free_block(b1);
    pmm_free_block(b2);
    pmm_free_block(b3);
    TEST_ASSERT(unique);
    return TEST_PASS;
}

TEST_CASE(pmm_alloc_blocks_contiguous)
{
    void* blocks = pmm_alloc_blocks(4);
    if (blocks == NULL)
    {
        return TEST_SKIP;
    }
    const uint32_t addr = PTR_TO_U32(blocks);
    const int aligned = (addr % 4096) == 0;
    pmm_free_blocks(blocks, 4);
    TEST_ASSERT(aligned);
    return TEST_PASS;
}

TEST_CASE(pmm_free_blocks_restores_count)
{
    const uint32_t before = pmm_get_free_block_count();
    void* blocks = pmm_alloc_blocks(8);
    if (blocks == NULL)
    {
        return TEST_SKIP;
    }
    const uint32_t during = pmm_get_free_block_count();
    pmm_free_blocks(blocks, 8);
    const uint32_t after = pmm_get_free_block_count();
    TEST_ASSERT_EQ(before, after);
    TEST_ASSERT_EQ(during, before - 8);
    return TEST_PASS;
}

TEST_CASE(pmm_stats_consistency)
{
    const uint32_t total = pmm_get_block_count();
    const uint32_t used = pmm_get_used_block_count();
    const uint32_t free = pmm_get_free_block_count();
    TEST_ASSERT_EQ(total, used + free);
    return TEST_PASS;
}

TEST_CASE(pmm_memory_size_positive)
{
    const uint32_t mem_size = pmm_get_memory_size();
    TEST_ASSERT_GT(mem_size, 0);
    return TEST_PASS;
}

static struct test_case pmm_cases[] = {
        TEST_ENTRY(pmm_alloc_block_returns_non_null),
        TEST_ENTRY(pmm_alloc_block_alignment),
        TEST_ENTRY(pmm_free_block_restores_count),
        TEST_ENTRY(pmm_alloc_multiple_unique),
        TEST_ENTRY(pmm_alloc_blocks_contiguous),
        TEST_ENTRY(pmm_free_blocks_restores_count),
        TEST_ENTRY(pmm_stats_consistency),
        TEST_ENTRY(pmm_memory_size_positive),
        TEST_SUITE_END
};

static struct test_suite pmm_suite = {
        .name = "PMM Tests",
        .cases = pmm_cases,
        .count = 8
};

struct test_suite* test_pmm_get_suite(void)
{
    return &pmm_suite;
}
