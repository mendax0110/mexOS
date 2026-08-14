#include "string.h"
#include "../../shared/types.h"
#include "../diag/panic.h"

/**
 * @brief 64-bit unsigned division: return n / d
 * @param n The numerator
 * @param d The denominator
 * @return A 64-bit unsigned integer representing the quotient of n divided by d
 */
uint64_t __udivdi3(const uint64_t n, const uint64_t d)
{
    if (d == 0)
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Division by zero: %llu / %llu", n, d);
        kernel_panic(buffer);
    }

    uint64_t quotient = 0;
    uint64_t remainder = 0;

    for (int i = 63; i >= 0; --i)
    {
        remainder <<= 1;
        remainder |= (n >> i) & 1;
        if (remainder >= d)
        {
            remainder -= d;
            quotient |= (1ULL << i);
        }
    }
    return quotient;
}

/**
 * @brief 64-bit signed division: return n / d
 * @param n The numerator
 * @param d The denominator
 * @return A 64-bit signed integer representing the quotient of n divided by d
 */
int64_t __divdi3(const int64_t n, const int64_t d)
{
    if (d == 0)
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Division by zero: %lld / %lld", n, d);
        kernel_panic(buffer);
    }

    const uint64_t un = (n < 0) ? (uint64_t)(-n) : (uint64_t)n;
    const uint64_t ud = (d < 0) ? (uint64_t)(-d) : (uint64_t)d;
    const uint64_t quot = __udivdi3(un, ud);

    if ((n < 0) ^ (d < 0))
    {
        return - (int64_t)quot;
    }
    return (int64_t)quot;
}

/**
 * @brief 64-bit unsigned modulus: return n % d
 * @param n The numerator
 * @param d The denominator
 * @return A 64-bit unsigned integer representing the remainder of n divided by d
 */
uint64_t __umoddi3(const uint64_t n, const uint64_t d)
{
    if (d == 0)
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Modulo by zero: %llu %% %llu", n, d);
        kernel_panic(buffer);
    }

    uint64_t remainder = 0;
    for (int i = 63; i >= 0; --i)
    {
        remainder <<= 1;
        remainder |= (n >> i) & 1;
        if (remainder >= d)
        {
            remainder -= d;
        }
    }
    return remainder;
}

/**
 * @brief 64-bit unsigned multiplication: return a * b
 * @param a The first operand
 * @param b The second operand
 * @return A 64-bit unsigned integer representing the product of a and b
 */
uint64_t __umuldi3(const uint64_t a, const uint64_t b)
{
    const uint32_t a_lo = (uint32_t)a;
    const uint32_t a_hi = (uint32_t)(a >> 32);
    const uint32_t b_lo = (uint32_t)b;
    const uint32_t b_hi = (uint32_t)(b >> 32);

    const uint64_t a_lo_b_lo = (uint64_t)a_lo * b_lo;
    const uint64_t a_hi_b_lo = (uint64_t)a_hi * b_lo;
    const uint64_t a_lo_b_hi = (uint64_t)a_lo * b_hi;

    const uint64_t mid = a_hi_b_lo + a_lo_b_hi;
    return a_lo_b_lo + (mid << 32);
}

/**
 * @brief 64-bit signed multiplication: return a * b
 * @param a The first operand
 * @param b The second operand
 * @return A 64-bit signed integer representing the product of a and b
 */
int64_t __muldi3(const int64_t a, const int64_t b)
{
    const uint64_t ua = (uint64_t)(a < 0 ? -a : a);
    const uint64_t ub = (uint64_t)(b < 0 ? -b : b);
    const uint64_t result = __umuldi3(ua, ub);
    if ((a < 0) ^ (b < 0))
    {
        return -(int64_t)result;
    }
    return (int64_t)result;
}