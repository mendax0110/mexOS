#ifndef KERNEL_MATH_H
#define KERNEL_MATH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Macro to find the minimum
 * @param a The first value
 * @param b The second value
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief Macro to find the maximum
 * @param a The first value
 * @param b The second value
 */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Macro to clamp a value between two bounds
 * @param x The value to clamp
 * @param min The minimum value
 * @param max The maximum value
 */
#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))

/**
 * @brief Macro to find the absolute value
 * @param x The value to get the abs from
 */
#define ABS(x) ((x) < 0 ? -(x) : (x))

/**
 * @brief Macro to perform integer division with ceiling
 * @param a The first value
 * @param b The 2nd value
 */
#define DIV_CEIL(a, b) (((a) + (b) - 1) / (b))

/**
 * @brief Macro to check if a number is a power of two
 * @param x The number to check
 */
#define IS_POWER_OF_TWO(x) ((x) != 0 && ((x) & ((x) - 1)) == 0))

/**
 * @brief Macro to align a value up to the nearest multiple of a given alignment
 * @param x The value to align
 * @param align The alignment
 */
#define ALIGN_UP(x, align) (DIV_CEIL((x), (align)) * (align))

/**
 * @brief Macro to align a value down to the nearest multiple of a given alignment
 * @param x The value to align
 * @param align The alignment
 */
#define ALIGN_DOWN(x, align) ((x) / (align) * (align))

/**
 * @brief Macro to swap two values of the same type
 * @param a The first value
 * @param b The second value
 */
#define SWAP(a, b)              \
    do                          \
    {                           \
        typeof(a) _tmp = (a);   \
        (a) = (b);              \
        (b) = _tmp;             \
    } while (0)

/**
 * @brief Converts a value to hexadecimal representation
 * @param c The value to convert
 * @return The converted hex value
 */
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

/**
 * @brief Converts a value from hexadecimal representation
 * @param str The input value to convert
 * @param len The length of the value
 * @return The converted decimal value
 */
uint32_t inline from_hex(const char* str, const int len)
{
    uint32_t result = 0;
    for (int i = 0; i < len; i++)
    {
        const uint32_t hex_val = to_hex(str[i]);
        if (hex_val == LIMIT)
        {
            return LIMIT;
        }
        result = (result << 4) | hex_val;
    }
    return result;
}

#ifdef __cplusplus
}
#endif

#endif // KERNEL_MATH_H
