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