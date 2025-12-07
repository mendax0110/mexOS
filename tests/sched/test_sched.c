#include "test_sched.h"
#include "../../kernel/sched/sched.h"

static volatile int test_task_ran = 0;

static void dummy_task_entry(void)
{
    test_task_ran = 1;
    while(1)
    {
        sched_yield();
    }
}

TEST_CASE(sched_get_current_not_null)
{
    const struct task* current = sched_get_current();
    TEST_ASSERT_NOT_NULL(current);
    return TEST_PASS;
}

TEST_CASE(sched_current_is_running)
{
    const struct task* current = sched_get_current();
    TEST_ASSERT_NOT_NULL(current);
    TEST_ASSERT_EQ(current->state, TASK_RUNNING);
    return TEST_PASS;
}

TEST_CASE(sched_task_list_not_empty)
{
    const struct task* list = sched_get_task_list();
    TEST_ASSERT_NOT_NULL(list);
    return TEST_PASS;
}

TEST_CASE(sched_idle_task_exists)
{
    const struct task* idle = sched_get_idle_task();
    TEST_ASSERT_NOT_NULL(idle);
    return TEST_PASS;
}

TEST_CASE(sched_task_create_kernel)
{
    const struct task* t = task_create(dummy_task_entry, 5, true);
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQ(t->state, TASK_READY);
    TEST_ASSERT_EQ(t->priority, 5);
    TEST_ASSERT_TRUE(t->kernel_mode);
    task_destroy(t->id);
    return TEST_PASS;
}

TEST_CASE(sched_task_create_unique_id)
{
    const struct task* t1 = task_create(dummy_task_entry, 5, true);
    const struct task* t2 = task_create(dummy_task_entry, 5, true);
    TEST_ASSERT_NOT_NULL(t1);
    TEST_ASSERT_NOT_NULL(t2);
    TEST_ASSERT_NEQ(t1->id, t2->id);
    task_destroy(t1->id);
    task_destroy(t2->id);
    return TEST_PASS;
}

TEST_CASE(sched_task_find_valid)
{
    const struct task* t = task_create(dummy_task_entry, 5, true);
    TEST_ASSERT_NOT_NULL(t);
    const struct task* found = task_find(t->pid);
    TEST_ASSERT_EQ(found, t);
    task_destroy(t->id);
    return TEST_PASS;
}

TEST_CASE(sched_task_find_invalid)
{
    const struct task* found = task_find(99999);
    TEST_ASSERT_NULL(found);
    return TEST_PASS;
}

TEST_CASE(sched_task_exit_zombie)
{
    const struct task* t = task_create(dummy_task_entry, 5, true);
    TEST_ASSERT_NOT_NULL(t);
    task_exit(t->id, 42);
    TEST_ASSERT_EQ(t->state, TASK_ZOMBIE);
    TEST_ASSERT_EQ(t->exit_code, 42);
    task_destroy(t->id);
    return TEST_PASS;
}

TEST_CASE(sched_task_count)
{
    const struct task* t = sched_get_task_list();
    int count = 0;
    while (t)
    {
        count++;
        t = t->next;
    }
    TEST_ASSERT_GE(count, 2);
    return TEST_PASS;
}

static struct test_case sched_cases[] = {
        TEST_ENTRY(sched_get_current_not_null),
        TEST_ENTRY(sched_current_is_running),
        TEST_ENTRY(sched_task_list_not_empty),
        TEST_ENTRY(sched_idle_task_exists),
        TEST_ENTRY(sched_task_create_kernel),
        TEST_ENTRY(sched_task_create_unique_id),
        TEST_ENTRY(sched_task_find_valid),
        TEST_ENTRY(sched_task_find_invalid),
        TEST_ENTRY(sched_task_exit_zombie),
        TEST_ENTRY(sched_task_count),
        TEST_SUITE_END
};

static struct test_suite sched_suite = {
        .name = "Scheduler Tests",
        .cases = sched_cases,
        .count = 11
};

struct test_suite* test_sched_get_suite(void)
{
    return &sched_suite;
}
