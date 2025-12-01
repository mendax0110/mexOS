#include "console.h"
#include "serial.h"

static uint16_t* vga_buffer;
static uint8_t console_color;
static uint32_t console_row;
static uint32_t console_col;

static inline uint8_t vga_entry_color(const uint8_t fg, const uint8_t bg)
{
    return fg | bg << 4;
}

static inline uint16_t vga_entry(const unsigned char c, const uint8_t color)
{
    return (uint16_t)c | (uint16_t)color << 8;
}

void console_init(void)
{
    vga_buffer = (uint16_t*)VGA_MEMORY;
    console_color = vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK);
    console_row = 0;
    console_col = 0;
    console_clear();
    serial_init();
}

void console_clear(void)
{
    for (uint32_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < VGA_WIDTH; x++)
        {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', console_color);
        }
    }
    console_row = 0;
    console_col = 0;
}

static void console_scroll(void)
{
    for (uint32_t y = 0; y < VGA_HEIGHT - 1; y++)
    {
        for (uint32_t x = 0; x < VGA_WIDTH; x++)
        {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (uint32_t x = 0; x < VGA_WIDTH; x++)
    {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', console_color);
    }
    console_row = VGA_HEIGHT - 1;
}

void console_putchar(char c)
{
    serial_write(c);

    if (c == '\n')
    {
        console_col = 0;
        if (++console_row >= VGA_HEIGHT)
        {
            console_scroll();
        }
        return;
    }
    if (c == '\r')
    {
        console_col = 0;
        return;
    }
    if (c == '\b')
    {
        if (console_col > 0)
        {
            console_col--;
        }
        return;
    }
    if (c == '\t')
    {
        console_col = (console_col + 8) & ~7;
        if (console_col >= VGA_WIDTH)
        {
            console_col = 0;
            if (++console_row >= VGA_HEIGHT)
            {
                console_scroll();
            }
        }
        return;
    }

    vga_buffer[console_row * VGA_WIDTH + console_col] = vga_entry(c, console_color);
    if (++console_col >= VGA_WIDTH)
    {
        console_col = 0;
        if (++console_row >= VGA_HEIGHT)
        {
            console_scroll();
        }
    }
}

void console_write(const char* str)
{
    while (*str)
    {
        console_putchar(*str++);
    }
}

void console_write_hex(uint32_t val)
{
    console_write("0x");
    char buf[9];
    for (int i = 7; i >= 0; i--)
    {
        const uint8_t digit = val & 0xF;
        buf[i] = digit < 10 ? '0' + digit : 'A' + digit - 10;
        val >>= 4;
    }
    buf[8] = '\0';
    console_write(buf);
}

void console_write_dec(uint32_t val)
{
    char buf[12];
    char* ptr = buf + 11;
    *ptr = '\0';

    if (val == 0)
    {
        console_putchar('0');
        return;
    }

    while (val > 0)
    {
        *--ptr = '0' + (val % 10);
        val /= 10;
    }
    console_write(ptr);
}

void console_set_color(const uint8_t fg, const uint8_t bg)
{
    console_color = vga_entry_color(fg, bg);
}
