#ifndef KERNEL_MATH_H
#define KERNEL_MATH_H

#include "types.h"

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

#endif // KERNEL_MATH_H
