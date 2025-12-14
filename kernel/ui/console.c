#include "console.h"
#include "vterm.h"
#include "../drivers/char/serial.h"

static uint8_t vterm_initialized = 0;

void console_init(void)
{
    serial_init();
    vterm_init();
    vterm_initialized = 1;
}

void console_clear(void)
{
    if (vterm_initialized)
    {
        vterm_clear(vterm_get_active());
    }
}

void console_putchar(char c)
{
    if (vterm_initialized)
    {
        vterm_putchar(vterm_get_active(), c);
    }
    else
    {
        serial_write(c);
    }
}

void console_write(const char* str)
{
    if (vterm_initialized)
    {
        vterm_write(vterm_get_active(), str);
    }
    else
    {
        while (*str)
        {
            serial_write(*str++);
        }
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
    if (vterm_initialized)
    {
        vterm_write_dec(vterm_get_active(), val);
    }
    else
    {
        char buf[12];
        char* ptr = buf + 11;
        *ptr = '\0';

        if (val == 0)
        {
            serial_write('0');
            return;
        }

        while (val > 0)
        {
            *--ptr = '0' + (val % 10);
            val /= 10;
        }
        while (*ptr)
        {
            serial_write(*ptr++);
        }
    }
}

void console_set_color(const uint8_t fg, const uint8_t bg)
{
    if (vterm_initialized)
    {
        vterm_set_color(vterm_get_active(), fg, bg);
    }
}
