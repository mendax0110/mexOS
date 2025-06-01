#include "mexKernel.h"

VGATerminal terminal;

void VGATerminal::initialize()
{
    buffer = (uint16_t*)VGA_MEMORY;
    row = 0;
    column = 0;
    color = 0x0F;
    for (uint32_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < VGA_WIDTH; x++)
        {
            buffer[y * VGA_WIDTH + x] = ' ' | (color << 8);
        }
    }
}

void VGATerminal::putchar(char c)
{
    if (c == '\n')
    {
        column = 0;
        if (++row == VGA_HEIGHT)
        {
            row = 0;
        }
        return;
    }

    buffer[row * VGA_WIDTH + column] = c | (color << 8);
    if (++column == VGA_WIDTH)
    {
        column = 0;
        if (++row == VGA_HEIGHT)
        {
            row = 0;
        }
    }
}

void VGATerminal::write(const char* str)
{
    while (*str)
    {
        putchar(*str++);
    }
}

void System::syscall(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    Kernel* kernel = Kernel::instance();
    switch (num)
    {
        case SYS_WRITE:
            kernel->terminal().write((const char*)arg1);
            break;
    }
}

VGATerminal& Kernel::terminal()
{
    return vgaTerminal;
}