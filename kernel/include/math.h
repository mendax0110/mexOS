#ifndef KERNEL_MATH_H
#define KERNEL_MATH_H

#include "types.h"
#include "cast.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define DIV_CEIL(a, b) (((a) + (b) - 1) / (b))

#define IS_POWER_OF_TWO(x) ((x) != 0 && ((x) & ((x) - 1)) == 0))

#define ALIGN_UP(x, align) (DIV_CEIL((x), (align)) * (align))

#define ALIGN_DOWN(x, align) ((x) / (align) * (align))

#define SWAP(a, b)              \
    do                          \
    {                           \
        typeof(a) _tmp = (a);   \
        (a) = (b);              \
        (b) = _tmp;             \
    } while (0)


uint32_t inline to_hex(const char c)
{
    if (c >= '0' && c <= '9')
    {
        return (uint32_t)(c - '0');
    }
    if (c >= 'A' && c <= 'F')
    {
        return (uint32_t)(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f')
    {
        return (uint32_t)(c - 'a' + 10);
    }
    return LIMIT;
}
#define TO_HEX(c) to_hex(c)

uint32_t inline from_hex(const char* str, const int len)
{
    uint32_t result = 0;
    for (int i = 0; i < len; i++)
    {
        const uint32_t hex_val = TO_HEX(str[i]);
        if (hex_val == LIMIT)
        {
            return LIMIT;
        }
        result = (result << 4) | hex_val;
    }
    return result;
}
#define FROM_HEX(str, len) from_hex((str), (len))

#ifdef __cplusplus
}
#endif

#endif // KERNEL_MATH_H