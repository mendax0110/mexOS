#include "../../shared/types.h"
#include "../../kernel/include/addr.h"
#include "../../kernel/include/bitops.h"
#include "../../kernel/mm/heap.h"
#include "../test_framework.h"
#include "test_types.h"

TEST_CASE(test_type_sizes)
{
    TEST_ASSERT(sizeof(uint8_t) == 1);
    TEST_ASSERT(sizeof(uint16_t) == 2);
    TEST_ASSERT(sizeof(uint32_t) == 4);
    TEST_ASSERT(sizeof(uint64_t) == 8);
    TEST_ASSERT(sizeof(int8_t) == 1);
    TEST_ASSERT(sizeof(int16_t) == 2);
    TEST_ASSERT(sizeof(int32_t) == 4);
    TEST_ASSERT(sizeof(int64_t) == 8);
    TEST_ASSERT(sizeof(size_t) == 4);
    TEST_ASSERT(sizeof(ssize_t) == 4);
    TEST_ASSERT(sizeof(pid_t) == 4);
    TEST_ASSERT(sizeof(tid_t) == 4);
    TEST_ASSERT(sizeof(uintptr_t) == 8 || sizeof(uintptr_t) == 4);
    TEST_ASSERT(sizeof(va_list) == sizeof(void*));
    TEST_ASSERT(sizeof(float32_t) == 4);
    TEST_ASSERT(sizeof(float64_t) == 8);
    return TEST_PASS;
}

TEST_CASE(test_null_and_bool)
{
    const void* ptr = NULL;
    TEST_ASSERT(ptr == 0);

    const bool btrue = true;
    const bool bfalse = false;
    TEST_ASSERT(btrue == 1);
    TEST_ASSERT(bfalse == 0);
    return TEST_PASS;
}

TEST_CASE(test_pointer_integer_macros)
{
    const uint32_t x = 0x12345678;
    void* ptr = PTR_FROM_U32(x);
    TEST_ASSERT(PTR_TO_U32(ptr) == x);

    const char* cptr = CONST_CHAR_FROM_U32(x);
    TEST_ASSERT((uintptr_t)cptr == x);

    char* cp = CHAR_FROM_U32(x);
    TEST_ASSERT((uintptr_t)cp == x);

    const uint32_t val = FUNC_PTR_TO_U32((void*)0x87654321);
    TEST_ASSERT(val == 0x87654321);

    struct dummy { int a; };
    struct dummy* dptr = PTR_FROM_U32_TYPED(struct dummy, 0xAABBCCDD);
    TEST_ASSERT(PTR_TO_U32(dptr) == 0xAABBCCDD);

    void* casted_ptr = PTR_CAST(void*, 0x11223344);
    TEST_ASSERT(PTR_TO_U32(casted_ptr) == 0x11223344);

    const uint32_t flag = BIT_MASK(0xF0F0, 0x0F0F);
    TEST_ASSERT(flag == (0xF0F0 & 0x0F0F));
    return TEST_PASS;
}

TEST_CASE(test_alignment_macro)
{
    struct ALIGNED(16) { uint8_t a; uint32_t b; } *s16 = kmalloc_aligned(sizeof(*s16), 16);
    struct ALIGNED(8)  { uint8_t a; uint32_t b; } *s8  = kmalloc_aligned(sizeof(*s8), 8);

    if (!s16 || !s8)
    {
        if (s16) kfree_aligned(s16);
        if (s8) kfree_aligned(s8);
        return TEST_SKIP;
    }

    TEST_ASSERT(((uintptr_t)s16 % 16) == 0);
    TEST_ASSERT(((uintptr_t)s8 % 8) == 0);

    kfree_aligned(s16);
    kfree_aligned(s8);
    return TEST_PASS;
}

TEST_CASE(test_uintptr_arithmetic)
{
    const uintptr_t x = 0x1000;
    const uintptr_t y = 0x200;
    const uintptr_t sum = x + y;
    const uintptr_t diff = x - y;
    TEST_ASSERT(sum == 0x1200);
    TEST_ASSERT(diff == 0x0E00);
    return TEST_PASS;
}

TEST_CASE(test_bit_flag_edge)
{
    TEST_ASSERT(BIT_FLAG(0xFFFF, 0) == 1);
    TEST_ASSERT(BIT_FLAG(0x0, 0) == 0);
    TEST_ASSERT(BIT_FLAG(0x1234, 0) == 0);
    TEST_ASSERT(BIT_FLAG(0x1234, 2) == 1);
    TEST_ASSERT(BIT_FLAG(0xFFFF, 15) == 1);
    TEST_ASSERT(BIT_FLAG(0x0000, 15) == 0);

    return TEST_PASS;
}

TEST_CASE(test_pointer_cast_edge)
{
    const void* null_ptr = PTR_CAST(void*, 0);
    TEST_ASSERT(null_ptr == NULL);

    struct dummy { int a; };
    const struct dummy* dptr = PTR_FROM_U32_TYPED(struct dummy, 0);
    TEST_ASSERT(dptr == NULL);

    return TEST_PASS;
}

TEST_CASE(test_packed_struct)
{
    struct PACKED s
    {
        uint8_t a;
        uint32_t b;
    };
    TEST_ASSERT(sizeof(struct s) == 5);
    return TEST_PASS;
}

TEST_CASE(test_aligned_heap)
{
    void* ptr16 = kmalloc_aligned(64, 16);
    void* ptr32 = kmalloc_aligned(64, 32);
    if (!ptr16 || !ptr32)
    {
        if(ptr16) kfree_aligned(ptr16);
        if(ptr32) kfree_aligned(ptr32);
        return TEST_SKIP;
    }

    TEST_ASSERT(((uintptr_t)ptr16 % 16) == 0);
    TEST_ASSERT(((uintptr_t)ptr32 % 32) == 0);

    kfree_aligned(ptr16);
    kfree_aligned(ptr32);
    return TEST_PASS;
}

TEST_CASE(test_pointer_round_trip)
{
    const uint32_t val = 0xDEADBEEF;
    void* ptr = PTR_FROM_U32(val);
    const uint32_t back = PTR_TO_U32(ptr);
    TEST_ASSERT(back == val);
    return TEST_PASS;
}

TEST_CASE(test_char_pointer_round_trip)
{
    void* base_ptr = kmalloc(16);
    if (!base_ptr) return TEST_SKIP;

    const uint32_t val = PTR_TO_U32(base_ptr);
    const char* cptr = CONST_CHAR_FROM_U32(val);
    char* ptr = CHAR_FROM_U32(val);

    TEST_ASSERT((uintptr_t)cptr == val);
    TEST_ASSERT((uintptr_t)ptr == val);

    kfree(base_ptr);
    return TEST_PASS;
}

TEST_CASE(test_unsigned_integer_num_calc)
{
    const uint32_t a = 10;
    const uint32_t b = 20;
    const uint32_t sum = a + b;
    const uint32_t diff = b - a;
    const uint32_t prod = a * b;
    const uint32_t quot = b / a;

    TEST_ASSERT_EQ(sum, 30U);
    TEST_ASSERT_EQ(diff, 10U);
    TEST_ASSERT_EQ(prod, 200U);
    TEST_ASSERT_EQ(quot, 2U);

    return TEST_PASS;
}

TEST_CASE(test_signed_integer_num_calc)
{
    const int32_t a = -10;
    const int32_t b = 20;
    const int32_t sum = a + b;
    const int32_t diff = b - a;
    const int32_t prod = a * b;
    const int32_t quot = b / a;

    TEST_ASSERT_EQ(sum, 10);
    TEST_ASSERT_EQ(diff, 30);
    TEST_ASSERT_EQ(prod, -200);
    TEST_ASSERT_EQ(quot, -2);

    return TEST_PASS;
}

TEST_CASE(test_float_num_calc)
{
    const float32_t a = 1.5f;
    const float32_t b = 2.5f;
    const float32_t sum = a + b;
    const float32_t diff = b - a;
    const float32_t prod = a * b;
    const float32_t quot = b / a;

    TEST_ASSERT_EQ(sum, 4.0f);
    TEST_ASSERT_EQ(diff, 1.0f);
    TEST_ASSERT_EQ(prod, 3.75f);
    TEST_ASSERT_EQ(quot, (float32_t)(5.0 / 3.0));

    return TEST_PASS;
}

TEST_CASE(test_64_bit_unsigned_integer_num_calc)
{
    const uint64_t a = 1000000000ULL;
    const uint64_t b = 2000000000ULL;
    const uint64_t sum = a + b;
    const uint64_t diff = b - a;
    const uint64_t prod = a * b;
    const uint64_t quot = b / a;

    TEST_ASSERT_EQ(sum, 3000000000ULL);
    TEST_ASSERT_EQ(diff, 1000000000ULL);
    TEST_ASSERT_EQ(prod, 2000000000000000000ULL);
    TEST_ASSERT_EQ(quot, 2ULL);

    return TEST_PASS;
}

TEST_CASE(test_64_bit_signed_integer_num_calc)
{
    const int64_t a = -1000000000LL;
    const int64_t b = 2000000000LL;
    const int64_t sum = a + b;
    const int64_t diff = b - a;
    const int64_t prod = a * b;
    const int64_t quot = b / a;

    TEST_ASSERT_EQ(sum, 1000000000LL);
    TEST_ASSERT_EQ(diff, 3000000000LL);
    TEST_ASSERT_EQ(prod, -2000000000000000000LL);
    TEST_ASSERT_EQ(quot, -2LL);

    return TEST_PASS;
}

TEST_CASE(test_64_bit_float_num_calc)
{
    const float64_t a = 1.5;
    const float64_t b = 2.5;
    const float64_t sum = a + b;
    const float64_t diff = b - a;
    const float64_t prod = a * b;
    const float64_t quot = b / a;

    TEST_ASSERT_EQ(sum, 4.0);
    TEST_ASSERT_EQ(diff, 1.0);
    TEST_ASSERT_EQ(prod, 3.75);
    TEST_ASSERT_EQ(quot, (5.0 / 3.0));

    return TEST_PASS;
}

static struct test_case cast_cases[] = {
        TEST_ENTRY(test_type_sizes),
        TEST_ENTRY(test_null_and_bool),
        TEST_ENTRY(test_pointer_integer_macros),
        TEST_ENTRY(test_alignment_macro),
        TEST_ENTRY(test_uintptr_arithmetic),
        TEST_ENTRY(test_bit_flag_edge),
        TEST_ENTRY(test_pointer_cast_edge),
        TEST_ENTRY(test_packed_struct),
        TEST_ENTRY(test_aligned_heap),
        TEST_ENTRY(test_pointer_round_trip),
        TEST_ENTRY(test_char_pointer_round_trip),
        TEST_ENTRY(test_unsigned_integer_num_calc),
        TEST_ENTRY(test_signed_integer_num_calc),
        TEST_ENTRY(test_float_num_calc),
        TEST_ENTRY(test_64_bit_unsigned_integer_num_calc),
        TEST_ENTRY(test_64_bit_signed_integer_num_calc),
        TEST_ENTRY(test_64_bit_float_num_calc),
        TEST_SUITE_END
};

static struct test_suite cast_suite = TEST_SUITE("Cast & Types Full Tests", cast_cases);

struct test_suite* test_types_get_suite(void)
{
    return &cast_suite;
}
