#ifndef USER_RUNTIME_H
#define USER_RUNTIME_H

#include "syscall.h"
#include "../shared/string_utils.h"

/**
 * @brief Get the length of a string
 * @param str The string to get the length of
 * @return The length of the string
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
 * @brief Compare two strings
 * @param lhs The first string
 * @param rhs The second string
 * @return The difference between the first non-matching characters
 */
static inline int user_strcmp(const char* lhs, const char* rhs)
{
    while (*lhs && *lhs == *rhs)
    {
        lhs++;
        rhs++;
    }

    return (int)((unsigned char)*lhs - (unsigned char)*rhs);
}

/**
 * @brief Compare two memory regions
 * @param lhs The first region
 * @param rhs The second region
 * @param len The number of bytes to compare
 * @return The difference between the first non-matching bytes
 */
static inline int user_memcmp(const void* lhs, const void* rhs, const size_t len)
{
    const unsigned char* a = lhs;
    const unsigned char* b = rhs;

    for (size_t i = 0; i < len; i++)
    {
        if (a[i] != b[i])
        {
            return (int)a[i] - (int)b[i];
        }
    }

    return 0;
}

/**
 * @brief Compare two strings up to a maximum number of characters
 * @param lhs The first string
 * @param rhs The second string
 * @param len The maximum number of characters to compare
 * @return The difference between the first non-matching characters
 */
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

/**
 * @brief Copy memory from one location to another
 * @param dest The destination buffer
 * @param src The source buffer
 * @param len The number of bytes to copy
 * @return A pointer to the destination buffer
 */
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

/**
 * @brief Concatenate two strings
 * @param dest The destination string
 * @param src The source string
 * @return A pointer to the destination string
 */
static inline char* user_strcat(char* dest, const char* src)
{
    char* d = dest;
    while (*d)
    {
        d++;
    }

    while (*src)
    {
        *d++ = *src++;
    }
    *d = '\0';

    return dest;
}

/**
 * @brief Copy a string from source to destination
 * @param dest The destination string
 * @param src The source string
 * @return A pointer to the destination string
 */
static inline char* user_strcpy(char* dest, const char* src)
{
    char* d = dest;
    while (*src)
    {
        *d++ = *src++;
    }
    *d = '\0';

    return dest;
}

/**
 * @brief Fill memory with a constant value
 * @param dest The destination buffer
 * @param value The value to fill with
 * @param len The number of bytes to fill
 * @return A pointer to the destination buffer
 */
static inline void* user_memset(void* dest, const int value, const size_t len)
{
    unsigned char* d = dest;
    for (size_t i = 0; i < len; i++)
    {
        d[i] = (unsigned char)value;
    }

    return dest;
}

/**
 * @brief Check if two strings are equal
 * @param lhs The first string
 * @param rhs The second string
 * @return true if the strings are equal, false otherwise
 */
static inline bool user_streq(const char* lhs, const char* rhs)
{
    return user_strcmp(lhs, rhs) == 0;
}

/**
 * @brief Check if a string contains a slash
 * @param str The string to check
 * @return true if the string contains a slash, false otherwise
 */
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

/**
 * @brief Print a string to the standard output
 * @param str The string to print
 */
static inline void user_print(const char* str)
{
    write(STDOUT_FILENO, str, (int)user_strlen(str));
}

/**
 * @brief Clears terminal screen
 * @param str The string to print
 * @return The number of bytes written, or -1 on error
 */
static inline int32_t user_clear(const char* str)
{
    return write(STDOUT_FILENO, str, (int)user_strlen(str));
}

/**
 * @brief Print a single character to the standard output
 * @param c The character to print
 */
static inline void user_putc(const char c)
{
    write(STDOUT_FILENO, &c, 1);
}

/**
 * @brief Print a decimal number to the standard output
 * @param num The number to print
 */
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

/**
 * @brief Print a string followed by a newline to the standard output
 * @param str The string to print
 */
static inline void user_println(const char* str)
{
    user_print(str);
    user_putc('\n');
}

/**
 * @brief Get the current working directory path
 * @return The current working directory path, or "/" on root and error
 */
static inline const char* user_get_directory_path(void)
{
    static char cwd[128];
    if (getcwd(cwd, sizeof(cwd)) < 0)
    {
        return "/";
    }
    return cwd;
}

#endif