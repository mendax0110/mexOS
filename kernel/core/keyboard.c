#include "keyboard.h"
#include "vterm.h"
#include "../arch/i686/arch.h"
#include "../arch/i686/idt.h"

static unsigned char key_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t buffer_head = 0;
static volatile uint32_t buffer_tail = 0;

static const char scancode_ascii[] =
{
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0
};

static const char scancode_shift[] =
{
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0
};

static uint8_t shift_pressed = 0;
static uint8_t extended_scancode = 0;

static void keyboard_callback(struct registers* regs)
{
    (void)regs;
    const uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode == 0xE0)
    {
        extended_scancode = 1;
        return;
    }

    if (scancode == 0x2A || scancode == 0x36)
    {
        shift_pressed = 1;
        extended_scancode = 0;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6)
    {
        shift_pressed = 0;
        extended_scancode = 0;
        return;
    }

    if (scancode & 0x80)
    {
        extended_scancode = 0;
        return;
    }

    if (extended_scancode)
    {
        extended_scancode = 0;
        char special_key = 0;

        switch (scancode)
        {
            case 0x48: special_key = KEY_ARROW_UP; break;
            case 0x50: special_key = KEY_ARROW_DOWN; break;
            case 0x4B: special_key =KEY_ARROW_LEFT; break;
            case 0x47: special_key = KEY_HOME; break;
            case 0x4F: special_key = KEY_END; break;
        }

        if (special_key)
        {
            const uint32_t next_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
            if (next_tail != buffer_head)
            {
                key_buffer[buffer_tail] = special_key;
                buffer_tail = next_tail;;
            }
            return;
        }

        if (vterm_handle_switch(scancode))
        {
            return;
        }
    }
    else if (vterm_handle_switch(scancode))
    {
        return;
    }

    if (scancode < sizeof(scancode_ascii))
    {
        const char c = shift_pressed ? scancode_shift[scancode] : scancode_ascii[scancode];
        if (c)
        {
            const uint32_t next_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
            if (next_tail != buffer_head)
            {
                key_buffer[buffer_tail] = c;
                buffer_tail = next_tail;
            }
        }
    }
}

void keyboard_init(void)
{
    buffer_head = 0;
    buffer_tail = 0;
    register_interrupt_handler(33, keyboard_callback);
}

int keyboard_has_data(void)
{
    return buffer_head != buffer_tail;
}

unsigned char keyboard_getchar(void)
{
    while (buffer_head == buffer_tail)
    {
        hlt();
    }
    const unsigned char c = key_buffer[buffer_head];
    buffer_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
