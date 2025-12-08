#include "../include/string.h"

void* memset(void* dest, const int val, size_t len)
{
    uint8_t* d = (uint8_t*)dest;
    while (len--) *d++ = (uint8_t)val;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t len)
{
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (len--) *d++ = *s++;
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t len)
{
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    while (len--)
    {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(uint8_t*)s1 - *(uint8_t*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n)
{
    while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(uint8_t*)s1 - *(uint8_t*)s2;
}

char* strcpy(char* dest, const char* src)
{
    char* d = dest;
    while ((*d++ = *src++)) {}
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n)
{
    char* d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

char* strcat(char* dest, const char* src)
{
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++)) {}
    return dest;
}

char* strncat(char* dest, const char* src, size_t n)
{
    char* d = dest;
    while (*d) d++;
    while (n && (*d++ = *src++)) n--;
    if (n == 0) *d = '\0';
    return dest;
}

void int_to_str_pad(int value, char* str, int width)
{
    char temp[12];
    int i = 0;
    if (value == 0)
    {
        temp[i++] = '0';
    }
    else
    {
        while (value > 0 && i < 11)
        {
            temp[i++] = '0' + (value % 10);
            value /= 10;
        }
    }
    while (i < width && i < 11) temp[i++] = '0';
    for (int j = 0; j < i; ++j) str[j] = temp[i - j - 1];
    str[i] = '\0';
}

void int_to_hex_pad(uint32_t value, char* str, int width)
{
    static const char hex_chars[] = "0123456789ABCDEF";

    for (int i = width - 1; i >= 0; i--)
    {
        str[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    str[width] = '\0';
}

char* itoa(int value, char* str, const int base)
{
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (base < 2 || base > 36)
    {
        *str = '\0';
        return str;
    }

    char* ptr = str;
    char* ptr1 = str;

    int is_negative = 0;
    if (value < 0 && base == 10)
    {
        is_negative = 1;
        value = -value;
    }

    do
    {
        const int tmp_value = value;
        value /= base;
        *ptr++ = digits[tmp_value - value * base];
    } while (value);

    if (is_negative)
    {
        *ptr++ = '-';
    }

    *ptr-- = '\0';

    while (ptr1 < ptr)
    {
        const char tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}