#include "test_stress.h"
#include "../test_framework.h"
#include "../../kernel/mm/heap.h"
#include "../../kernel/sched/sched.h"
#include "../../kernel/lib/string.h"

TEST_CASE(stress_heap_fragmentation)
{
    void* ptrs[64];
    for (int i = 0; i < 64; i++)
    {
        ptrs[i] = kmalloc(32 + i);
        if (!ptrs[i])
        {
            return TEST_SKIP;
        }
    }

    for (int i = 0; i < 64; i++)
    {
        kfree(ptrs[i]);
        ptrs[i] = NULL;
    }

    void* big = kmalloc(512);
    TEST_ASSERT_NOT_NULL(big);
    kfree(big);
    return TEST_PASS;
}

TEST_CASE(stress_heap_alloc_free_cycle)
{
    const size_t sizes[] = { 1, 7, 16, 33, 64, 128, 255, 512, 1024, 4096 };
    const int n = sizeof(sizes) / sizeof(sizes[0]);

    for (int round = 0; round < 20; round++)
    {
        void* p = kmalloc(sizes[round % n]);
        TEST_ASSERT_NOT_NULL(p);
        memset(p, (uint8_t)(round & 0xFF), sizes[round % n]);
        const uint8_t* b = p;
        for (size_t j = 0; j < sizes[round % n]; j++)
        {
            if (b[j] != (uint8_t)(round & 0xFF))
            {
                kfree(p);
                TEST_ASSERT(0);
            }
        }
        kfree(p);
    }

    return TEST_PASS;
}

TEST_CASE(stress_heap_aligned_mixed)
{
    void* a16 = kmalloc_aligned(128, 16);
    void* a64 = kmalloc_aligned(128, 64);
    void* plain = kmalloc(64);
    void* a4096 = kmalloc_aligned(4096, 4096);

    TEST_ASSERT_NOT_NULL(a16);
    TEST_ASSERT_NOT_NULL(a64);
    TEST_ASSERT_NOT_NULL(plain);
    TEST_ASSERT_NOT_NULL(a4096);

    TEST_ASSERT(((uintptr_t)a16 % 16) == 0);
    TEST_ASSERT(((uintptr_t)a64 % 64) == 0);
    TEST_ASSERT(((uintptr_t)a4096 % 4096) == 0);

    memset(a16, 0xAA, 128);
    memset(a64, 0xBB, 128);
    memset(plain, 0xCC, 64);
    memset(a4096, 0xDD, 4096);

    const uint8_t* p = a16;
    for (int i = 0; i < 128; i++)
    {
        TEST_ASSERT(p[i] == 0xAA);
    }
    p = a64;
    for (int i = 0; i < 128; i++)
    {
        TEST_ASSERT(p[i] == 0xBB);
    }
    p = plain;
    for (int i = 0; i < 64; i++)
    {
        TEST_ASSERT(p[i] == 0xCC);
    }
    p = a4096;
    for (int i = 0; i < 4096; i++)
    {
        TEST_ASSERT(p[i] == 0xDD);
    }

    kfree_aligned(a16);
    kfree_aligned(a64);
    kfree(plain);
    kfree_aligned(a4096);

    return TEST_PASS;
}

TEST_CASE(stress_heap_exhaustion_recovery)
{
    void* ptrs[256];
    int count = 0;

    while (count < 256)
    {
        ptrs[count] = kmalloc(1024);
        if (!ptrs[count])
        {
            break;
        }
        count++;
    }

    TEST_ASSERT(count > 0);

    for (int i = 0; i < count; i++)
    {
        kfree(ptrs[i]);
    }

    void* p = kmalloc(64);
    TEST_ASSERT_NOT_NULL(p);
    kfree(p);

    return TEST_PASS;
}


static void dummy(void)
{
    while (1) sched_yield();
}

TEST_CASE(stress_sched_create_destroy_many)
{
    for (int i = 0; i < 32; i++)
    {
        const struct task* t = task_create(dummy, TASK_PRIORITY_LOW, true);
        TEST_ASSERT_NOT_NULL(t);
        TEST_ASSERT_EQ(t->state, TASK_READY);
        task_destroy(t->id);
    }

    return TEST_PASS;
}

TEST_CASE(stress_sched_exit_then_destroy)
{
    for (int i = 0; i < 16; i++)
    {
        const struct task* t = task_create(dummy, TASK_PRIORITY_LOW, true);
        TEST_ASSERT_NOT_NULL(t);
        task_exit(t->id, i);
        TEST_ASSERT_EQ(t->state, TASK_ZOMBIE);
        TEST_ASSERT_EQ(t->exit_code, i);
        task_destroy(t->id);
    }

    return TEST_PASS;
}

TEST_CASE(stress_sched_unique_ids)
{
    tid_t ids[16];

    for (int i = 0; i < 16; i++)
    {
        const struct task* t = task_create(dummy, TASK_PRIORITY_LOW, true);
        TEST_ASSERT_NOT_NULL(t);
        ids[i] = t->id;
        task_destroy(t->id);
    }

    for (int i = 0; i < 15; i++)
    {
        TEST_ASSERT_NEQ(ids[i], ids[i + 1]);
    }

    return TEST_PASS;
}

TEST_CASE(stress_sched_find_after_destroy)
{
    const struct task* t = task_create(dummy, TASK_PRIORITY_LOW, true);
    TEST_ASSERT_NOT_NULL(t);
    const tid_t id = t->id;
    const pid_t pid = t->pid;
    task_destroy(id);
    TEST_ASSERT_NULL(task_find(pid));
    return TEST_PASS;
}

static struct test_case stress_cases[] = {
    TEST_ENTRY(stress_heap_fragmentation),
    TEST_ENTRY(stress_heap_alloc_free_cycle),
    TEST_ENTRY(stress_heap_aligned_mixed),
    TEST_ENTRY(stress_heap_exhaustion_recovery),
    TEST_ENTRY(stress_sched_create_destroy_many),
    TEST_ENTRY(stress_sched_exit_then_destroy),
    TEST_ENTRY(stress_sched_unique_ids),
    TEST_ENTRY(stress_sched_find_after_destroy),
    TEST_SUITE_END
};

static struct test_suite stress_suite = TEST_SUITE("stress", stress_cases);

struct test_suite* test_stress_get_suite(void)
{
    return &stress_suite;
}