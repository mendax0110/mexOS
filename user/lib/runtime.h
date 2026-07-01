#ifndef USER_RUNTIME_H
#define USER_RUNTIME_H

#include "syscall.h"

/**
 * @brief User space strlen
 * @param str The string
 * @return The length
 */
static inline size_t user_strlen(const char* str)
{
    size_t len = 0;
    while (str[len])
    {
        len++;
    }
    return len;
}

/**
 * @brief User space print
 * @param str The string to print
 */
static inline void user_print(const char* str)
{
    write(str, (int)user_strlen(str));
}

/**
 * @brief User space print decimal
 * @param num The number to print
 */
static inline void user_print_dec(int num)
{
    if (num == 0)
    {
        write("0", 1);
        return;
    }

    char buf[12];
    int i = 0;
    int neg = 0;

    if (num < 0)
    {
        neg = 1;
        num = -num;
    }

    while (num > 0)
    {
        buf[i++] = (char)('0' + (num % 10));
        num /= 10;
    }

    if (neg)
    {
        buf[i++] = '-';
    }

    char out[12];
    int j = 0;
    while (i > 0)
    {
        out[j++] = buf[--i];
    }

    write(out, j);
}

#endif
