#include "../test_framework.h"
#include "test_rollback.h"
#include "../../kernel/include/cast.h"
#include "../lib/string.h"

static int rollback_called = 0;
static int lambda_called = 0;

static void named_rollback(void)
{
    rollback_called = 1;
}

TEST_CASE(rollback_named_fn_fires)
{
    rollback_called = 0;

    TRY_CTX("test_named", named_rollback)
    {
        ROLLBACK();
    }

    TEST_ASSERT_EQ(rollback_called, 1);
    return TEST_PASS;
}

TEST_CASE(rollback_named_fn_not_fired_on_success)
{
    rollback_called =0;

    TRY_CTX("test_no_rollback", named_rollback)
    {

    }

    TEST_ASSERT_EQ(rollback_called, 0);
    return TEST_PASS;
}

TEST_CASE(rollback_lambda_fires)
{
    lambda_called =0;

    TRY_CTX("test_lambda", LAMBDA(void, (void), {
        lambda_called = 1;
    }))
    {
        ROLLBACK();
    }

    TEST_ASSERT_EQ(lambda_called, 1);
    return TEST_PASS;
}

TEST_CASE(rollback_lambda_not_fired_on_success)
{
    lambda_called = 0;

    TRY_CTX("test_lambda_no_rollback", LAMBDA(void, (void), {
        lambda_called = 1;
    }))
    {

    }

    TEST_ASSERT_EQ(lambda_called, 0);
    return TEST_PASS;
}

TEST_CASE(rollback_null_does_not_crash)
{
    TRY_CTX("test_null_rollback", NULL)
    {
        ROLLBACK();
    }

    return TEST_PASS;
}

TEST_CASE(rollback_ctx_name_correct)
{
    TRY_CTX("my_ctx", NULL)
    {
        TEST_ASSERT_NOT_NULL(g_fault_ctx);
        TEST_ASSERT_EQ(strcmp(g_fault_ctx->name, "my_ctx"), 0);
    }

    return TEST_PASS;
}

TEST_CASE(rollback_ctx_popped_after_block)
{
    fault_ctx_t* before = g_fault_ctx;

    TRY_CTX("pop_ctx", NULL)
    {

    }

    TEST_ASSERT_EQ(g_fault_ctx, before);
    return TEST_PASS;
}

TEST_CASE(rollback_nested_ctx_order)
{
    int outer = 0;
    int inner = 0;

    static int* outer_ptr = NULL;
    static int* inner_ptr = NULL;
    outer_ptr = &outer;
    inner_ptr = &inner;

    TRY_CTX("outer", LAMBDA(void, (void), {*outer_ptr = 1;}))
    {
        TRY_CTX("inner", LAMBDA(void, (void), {*inner_ptr = 1;}))
        {
            ROLLBACK();
        }
    }

    TEST_ASSERT_EQ(inner, 1);
    TEST_ASSERT_EQ(outer, 0);
    return TEST_PASS;
}

TEST_CASE(rollback_lambda_called_once)
{
    static int count;
    count = 0;

    TRY_CTX("once_test", LAMBDA(void, (void), {count++;}))
    {
        ROLLBACK();
        ROLLBACK();
    }

    TEST_ASSERT_EQ(count, 1);
    return TEST_PASS;
}

static struct test_case rollback_cases[] = {
        TEST_ENTRY(rollback_named_fn_fires),
        TEST_ENTRY(rollback_named_fn_not_fired_on_success),
        TEST_ENTRY(rollback_lambda_fires),
        TEST_ENTRY(rollback_lambda_not_fired_on_success),
        TEST_ENTRY(rollback_null_does_not_crash),
        TEST_ENTRY(rollback_ctx_name_correct),
        TEST_ENTRY(rollback_ctx_popped_after_block),
        TEST_ENTRY(rollback_nested_ctx_order),
        TEST_ENTRY(rollback_lambda_called_once),
        TEST_SUITE_END
};

static struct test_suite rollback_suite = {
        .name = "Rollback Tests",
        .cases = rollback_cases,
        .count = 9
};

struct test_suite* test_rollback_get_suite(void)
{
    return &rollback_suite;
}