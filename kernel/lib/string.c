#include "lib/string.h"
#include "include/cast.h"

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

void* memset(void* dest, const int val, size_t len)
{
    ASSERT(dest != NULL);
    uint8_t* d = dest;
    while (len--) { *d++ = (uint8_t)val; }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t len)
{
    ASSERT(dest != NULL);
    ASSERT(src != NULL);
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (len--) { *d++ = *s++; }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t len)
{
    ASSERT(s1 != NULL);
    ASSERT(s2 != NULL);
    const uint8_t* p1 = s1;
    const uint8_t* p2 = s2;
    while (len--)
    {
        if (*p1 != *p2) { return *p1 - *p2; }
        p1++; p2++;
    }
    return 0;
}

void* memmov(void* dest, const void* src, size_t len)
{
    ASSERT(dest != NULL);
    ASSERT(src != NULL);
    uint8_t* d = dest;
    const uint8_t* s = src;
    if (d < s)
    {
        while (len--) *d++ = *s++;
    }
    else
    {
        d += len; s += len;
        while (len--) *--d = *--s;
    }
    return dest;
}

size_t strlen(const char* str)
{
    ASSERT(str != NULL);
    size_t len = 0;
    while (str[len]) { len++; }
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    ASSERT(s1 != NULL);
    ASSERT(s2 != NULL);
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(uint8_t*)s1 - *(uint8_t*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n)
{
    ASSERT(s1 != NULL);
    ASSERT(s2 != NULL);
    while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) { return 0; }
    return *(uint8_t*)s1 - *(uint8_t*)s2;
}

char* strcpy(char* dest, const char* src)
{
    ASSERT(dest != NULL);
    ASSERT(src != NULL);
    char* d = dest;
    while ((*d++ = *src++)) {}
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n)
{
    ASSERT(dest != NULL);
    ASSERT(src != NULL);
    char* d = dest;
    while (n && ((*d++ = *src++))) n--;
    while (n--) *d++ = '\0';
    return dest;
}

char* strcat(char* dest, const char* src)
{
    ASSERT(dest != NULL);
    ASSERT(src != NULL);
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++)) {}
    return dest;
}

char* strncat(char* dest, const char* src, size_t n)
{
    ASSERT(dest != NULL);
    ASSERT(src != NULL);
    char* d = dest;
    while (*d) d++;
    while (n && ((*d++ = *src++))) n--;
    if (n == 0) *d = '\0';
    return dest;
}

char* snprintf(char* str, const size_t size, const char* format, ...)
{
    ASSERT(str != NULL);
    ASSERT(format != NULL);

    if (size == 0) return str;

    va_list args = NULL;
    va_start(args, format);

    char* ptr = str;
    const char* end = str + size - 1;
    const char* fmt = format;

    while (*fmt && ptr < end)
    {
        if (*fmt != '%')
        {
            *ptr++ = *fmt++;
            continue;
        }

        fmt++;
        if (!*fmt) break;

        int zero_pad = 0;
        if (*fmt == '0') { zero_pad = 1; fmt++; }

        int width = 0;
        while (*fmt >= '0' && *fmt <= '9')
        {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        if (*fmt == 'd' || *fmt == 'u')
        {
            const int val = va_arg(args, int);
            char tmp[14];
            int_to_str_pad(val, tmp, width, zero_pad);
            for (const char* t = tmp; *t && ptr < end; t++) { *ptr++ = *t; }
        }
        else if (*fmt == 'x')
        {
            const uint32_t val = va_arg(args, uint32_t);
            char tmp[9];
            int w = (width > 0 && width <= 8) ? width : 8;
            int_to_hex_pad(val, tmp, w);
            for (const char* t = tmp; *t && ptr < end; t++) { *ptr++ = *t; }
        }
        else if (*fmt == 'p')
        {
            const uint32_t val = va_arg(args, uint32_t);
            if (ptr + 1 < end) { *ptr++ = '0'; }
            if (ptr + 1 < end) { *ptr++ = 'x'; }
            char tmp[9];
            int_to_hex_pad(val, tmp, 8);
            for (const char* t = tmp; *t && ptr < end; t++) { *ptr++ = *t; }
        }
        else if (*fmt == 's')
        {
            const char* s = va_arg(args, const char*);
            if (!s) s = "(null)";
            while (*s && ptr < end) { *ptr++ = *s++; }
        }
        else if (*fmt == 'c')
        {
            const char c = (char)va_arg(args, int);
            if (ptr < end) { *ptr++ = c; }
        }
        else if (*fmt == '%')
        {
            if (ptr < end) { *ptr++ = '%'; }
        }
        else
        {
            if (ptr < end) { *ptr++ = '%'; }
            if (ptr < end) { *ptr++ = *fmt; }
        }

        fmt++;
    }

    *ptr = '\0';
    va_end(args);
    return str;
}

void int_to_str_pad(int value, char* str, const int width, const int zero_pad)
{
    ASSERT(str != NULL);
    ASSERT(width >= 0);

    char temp[13];
    int i = 0;
    const int is_negative = (value < 0);

    if (is_negative) { value = -value; }

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

    for (int a = 0, b = i - 1; a < b; a++, b--)
    {
        char t = temp[a]; temp[a] = temp[b]; temp[b] = t;
    }
    temp[i] = '\0';

    int out = 0;
    const char pad_char = zero_pad ? '0' : ' ';
    int digits_and_sign = i + (is_negative ? 1 : 0);

    if (zero_pad && is_negative)
    {
        str[out++] = '-';
    }

    while (digits_and_sign++ < width)
    {
        str[out++] = pad_char;
    }

    if (!zero_pad && is_negative)
    {
        str[out++] = '-';
    }

    for (int j = 0; temp[j]; j++)
    {
        str[out++] = temp[j];
    }

    str[out] = '\0';
}

void int_to_hex_pad(uint32_t value, char* str, const int width)
{
    ASSERT(str != NULL);
    ASSERT(width > 0 && width <= 8);
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
    ASSERT(str != NULL);
    ASSERT(base >= 2 && base <= 36);
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

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