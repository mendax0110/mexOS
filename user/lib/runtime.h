#ifndef USER_RUNTIME_H
#define USER_RUNTIME_H

#include "syscall.h"

static inline size_t user_strlen(const char* str)
{
    size_t len = 0;
    while (str[len])
    {
        len++;
    }
    return len;
}

static inline int user_strcmp(const char* lhs, const char* rhs)
{
    while (*lhs && *lhs == *rhs)
    {
        lhs++;
        rhs++;
    }

    return (int)((unsigned char)*lhs - (unsigned char)*rhs);
}

static inline int user_strncmp(const char* lhs, const char* rhs, const size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (lhs[i] != rhs[i] || lhs[i] == '\0' || rhs[i] == '\0')
        {
            return (int)((unsigned char)lhs[i] - (unsigned char)rhs[i]);
        }
    }

    return 0;
}

static inline void* user_memcpy(void* dest, const void* src, const size_t len)
{
    unsigned char* d = dest;
    const unsigned char* s = src;

    for (size_t i = 0; i < len; i++)
    {
        d[i] = s[i];
    }

    return dest;
}

static inline void* user_memset(void* dest, const int value, const size_t len)
{
    unsigned char* d = dest;
    for (size_t i = 0; i < len; i++)
    {
        d[i] = (unsigned char)value;
    }

    return dest;
}

static inline bool user_streq(const char* lhs, const char* rhs)
{
    return user_strcmp(lhs, rhs) == 0;
}

static inline bool user_has_slash(const char* str)
{
    while (*str)
    {
        if (*str == '/')
        {
            return true;
        }
        str++;
    }

    return false;
}

static inline void user_print(const char* str)
{
    write(STDOUT_FILENO, str, (int)user_strlen(str));
}

static inline void user_putc(const char c)
{
    write(STDOUT_FILENO, &c, 1);
}

static inline void user_print_dec(int num)
{
    if (num == 0)
    {
        write(STDOUT_FILENO, "0", 1);
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

    write(STDOUT_FILENO, out, j);
}

static inline void user_println(const char* str)
{
    user_print(str);
    user_putc('\n');
}

#endif
