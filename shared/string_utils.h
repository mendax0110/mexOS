#ifndef SHARED_STRING_UTILS_H
#define SHARED_STRING_UTILS_H

#include "types.h"

/**
 * @brief Converts a value to hexadecimal representation
 * @param c The value to convert
 * @return The converted hex value
 */
static inline uint32_t to_hex(const char c)
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
static inline uint32_t from_hex(const char* str, const int len)
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
    bool overflowed = false;

    while (*str >= '0' && *str <= '9')
    {
        const int digit = *str++ - '0';
        if (value > (INT32_MAX - digit) / 10)
        {
            overflowed = true;
        }
        value = (value * 10) + digit;
    }

    if (overflowed)
    {
        return sign == 1 ? INT32_MAX : INT32_MIN;
    }

    return value * sign;
}

/**
 * @brief Copy a string into a fixed-size buffer.
 * @param dest Destination buffer
 * @param size Size of the destination buffer
 * @param src Source string
 * @return Number of characters copied, excluding the terminator
 */
static inline size_t copy_string(char* dest, const size_t size, const char* src)
{
    if (!dest || size == 0)
    {
        return 0;
    }

    size_t i = 0;
    if (src)
    {
        while (src[i] && i + 1 < size)
        {
            dest[i] = src[i];
            i++;
        }
    }

    dest[i] = '\0';
    return i;
}

/**
 * @brief Append a string to a fixed-size buffer.
 * @param dest Destination buffer
 * @param size Size of the destination buffer
 * @param src Source string
 * @return New string length, excluding the terminator
 */
static inline size_t append_string(char* dest, const size_t size, const char* src)
{
    if (!dest || size == 0)
    {
        return 0;
    }

    size_t pos = 0;
    while (pos < size && dest[pos])
    {
        pos++;
    }

    if (pos >= size)
    {
        return size;
    }

    return pos + copy_string(dest + pos, size - pos, src);
}

/**
 * @brief Return the basename portion of a path.
 * @param path Input path
 * @return Pointer to the basename within the input string
 */
static inline const char* path_basename(const char* path)
{
    if (!path)
    {
        return "";
    }

    const char* base = path;
    for (const char* p = path; *p; p++)
    {
        if (*p == '/')
        {
            base = p + 1;
        }
    }

    return base;
}

/**
 * @brief Append an unsigned decimal value to a fixed-size buffer.
 * @param dest Destination buffer
 * @param size Size of the destination buffer
 * @param value Value to append
 * @return New string length, excluding the terminator
 */
static inline size_t append_uint_dec(char* dest, const size_t size, uint32_t value)
{
    if (!dest || size == 0)
    {
        return 0;
    }

    size_t pos = 0;
    while (pos < size && dest[pos])
    {
        pos++;
    }

    if (pos >= size)
    {
        return size;
    }

    char tmp[10];
    size_t len = 0;
    do
    {
        tmp[len++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (value > 0U && len < sizeof(tmp));

    while (len > 0 && pos + 1 < size)
    {
        dest[pos++] = tmp[--len];
    }

    dest[pos] = '\0';
    return pos;
}

/**
 * @brief Append a signed decimal value to a fixed-size buffer.
 * @param dest Destination buffer
 * @param size Size of the destination buffer
 * @param value Value to append
 * @return New string length, excluding the terminator
 */
static inline size_t append_int_dec(char* dest, const size_t size, const int value)
{
    if (value < 0)
    {
        size_t pos = 0;
        while (pos < size && dest[pos])
        {
            pos++;
        }
        if (pos + 1 >= size)
        {
            return size;
        }
        dest[pos++] = '-';
        dest[pos] = '\0';
        return append_uint_dec(dest, size, (uint32_t)(-(value + 1)) + 1U);
    }

    return append_uint_dec(dest, size, (uint32_t)value);
}

#endif