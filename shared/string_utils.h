#ifndef SHARED_STRING_UTILS_H
#define SHARED_STRING_UTILS_H

#include "types.h"

/**
 * @brief Converts a value to hexadecimal representation
 * @param c The value to convert
 * @return The converted hex value
 */
inline uint32_t to_hex(const char c)
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
    return UINT32_INVALID;
}

/**
 * @brief Converts a value from hexadecimal representation
 * @param str The input value to convert
 * @param len The length of the value
 * @return The converted decimal value
 */
inline uint32_t from_hex(const char* str, const int len)
{
    uint32_t result = 0;
    for (int i = 0; i < len; i++)
    {
        const uint32_t hex_val = to_hex(str[i]);
        if (hex_val == UINT32_INVALID)
        {
            return UINT32_INVALID;
        }
        result = (result << 4) | hex_val;
    }
    return result;
}

/**
 * @brief Parse a signed decimal integer from a string
 * @param str The string to parse. Digits are consumed until the first non digit char
 * @return The parsed value, or 0 if no digits were present
 */
static inline int parse_int(const char* str)
{
    int sign = 1;

    if (*str == '-')
    {
        sign = -1;
        str++;
    }

    int value = 0;
    while (*str >= '0' && *str <= '9')
    {
        value = (value * 10) + (*str++ - '0');
    }

    return value * sign;
}

#endif