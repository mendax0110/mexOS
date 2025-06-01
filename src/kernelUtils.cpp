#include "kernelUtils.h"

const char* hex_to_str(uint32_t value)
{
    static char buffer[9];
    for (int i = 7; i >= 0; --i)
    {
        uint8_t digit = value & 0xF;
        buffer[i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value >>= 4;
    }
    buffer[8] = '\0';
    return buffer;
}

const char* int_to_str(uint32_t value)
{
    static char buffer[11];
    char* ptr = buffer + 10;
    *ptr = '\0';

    if (value == 0)
    {
        *(--ptr) = '0';
    }
    else
    {
        while (value > 0)
        {
            *(--ptr) = '0' + (value % 10);
            value /= 10;
        }
    }

    return ptr;
}

void* memcpy(void* dest, const void* src, uint32_t n)
{
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (uint32_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dest;
}

uint32_t strlen(const char* str)
{
    uint32_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const uint8_t*)s1 - *(const uint8_t*)s2;
}

void strcpy(char* dest, const char* src)
{
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void *kmalloc(uint32_t size)
{
    static uint8_t heap[4096];
    static uint32_t heap_index = 0;

    if (heap_index + size > sizeof(heap))
        return nullptr; // Out of memory

    void* ptr = &heap[heap_index];
    heap_index += size;
    return ptr;
}

void *memset(void* dest, int value, uint32_t size)
{
    char* d = (char*)dest;
    for (uint32_t i = 0; i < size; i++)
    {
        d[i] = (char)value;
    }
    return dest;
}