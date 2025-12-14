#include "vterm.h"
#include "console.h"
#include "../drivers/char/serial.h"
#include "../lib/log.h"
#include "../include/string.h"
#include "../arch/i686/arch.h"

#define VGA_MEMORY 0xB8000

static struct vterm terminals[VTERM_MAX_COUNT];
static uint8_t active_terminal = 0;
static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;

static const char* terminal_names[VTERM_MAX_COUNT] = {
        "console",
        "init",
        "user1",
        "user2"
};

static inline uint8_t vga_entry_color(const uint8_t fg, const uint8_t bg)
{
    return fg | (bg << 4);
}

static inline uint16_t vga_entry(const unsigned char c, const uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

void vterm_init(void)
{
    log_info("Initializing virtual terminals");

    for (int i = 0; i < VTERM_MAX_COUNT; i++)
    {
        struct vterm* vt = &terminals[i];

        memset(vt->buffer, 0, sizeof(vt->buffer));
        vt->cursor_row = 0;
        vt->cursor_col = 0;
        vt->scroll_offset = 0;
        vt->total_lines = 0;
        vt->color = vga_entry_color(7, 0);
        vt->owner_pid = -1;
        vt->active = (i == 0);
        strncpy(vt->name, terminal_names[i], 15);
        vt->name[15] = '\0';

        for (int j = 0; j < VTERM_BUFFER_SIZE; j++)
        {
            vt->buffer[j] = vga_entry(' ', vt->color);
        }
    }

    active_terminal = 0;
    vterm_refresh();

    log_info("Virtual terminals initialized (4 terminals, 200 line scrollback)");
}

struct vterm* vterm_get(const uint8_t id)
{
    if (id >= VTERM_MAX_COUNT)
    {
        return NULL;
    }
    return &terminals[id];
}

struct vterm* vterm_get_active(void)
{
    return &terminals[active_terminal];
}

uint8_t vterm_get_active_id(void)
{
    return active_terminal;
}

int vterm_switch(const uint8_t id)
{
    if (id >= VTERM_MAX_COUNT)
    {
        return -1;
    }

    if (id == active_terminal)
    {
        return 0;
    }

    terminals[active_terminal].active = false;
    active_terminal = id;
    terminals[active_terminal].active = true;

    vterm_refresh();

    return 0;
}

static void vterm_scroll(struct vterm* vt)
{
    for (uint32_t y = 0; y < VTERM_SCROLLBACK - 1; y++)
    {
        for (uint32_t x = 0; x < VTERM_WIDTH; x++)
        {
            vt->buffer[y * VTERM_WIDTH + x] = vt->buffer[(y + 1) * VTERM_WIDTH + x];
        }
    }

    for (uint32_t x = 0; x < VTERM_WIDTH; x++)
    {
        vt->buffer[(VTERM_SCROLLBACK - 1) * VTERM_WIDTH + x] = vga_entry(' ', vt->color);
    }

    if (vt->total_lines < VTERM_SCROLLBACK)
    {
        vt->total_lines++;
    }

    vt->cursor_row = VTERM_SCROLLBACK - 1;
    vt->scroll_offset = 0;
}

static void vterm_putchar_internal(struct vterm* vt, const char c, const bool do_refresh)
{
    if (vt == NULL)
    {
        return;
    }

    vt->scroll_offset = 0;

    if (c == '\n')
    {
        vt->cursor_col = 0;
        if (++vt->cursor_row >= VTERM_SCROLLBACK)
        {
            vterm_scroll(vt);
        }
        if (vt->total_lines < vt->cursor_row + 1)
        {
            vt->total_lines = vt->cursor_row + 1;
        }
    }
    else if (c == '\r')
    {
        vt->cursor_col = 0;
    }
    else if (c == '\b')
    {
        if (vt->cursor_col > 0)
        {
            vt->cursor_col--;
            vt->buffer[vt->cursor_row * VTERM_WIDTH + vt->cursor_col] = vga_entry(' ', vt->color);
        }
    }
    else if (c == '\t')
    {
        vt->cursor_col = (vt->cursor_col + 8) & ~7;
        if (vt->cursor_col >= VTERM_WIDTH)
        {
            vt->cursor_col = 0;
            if (++vt->cursor_row >= VTERM_SCROLLBACK)
            {
                vterm_scroll(vt);
            }
        }
    }
    else if (c >= 0x20 && c < 0x7F)
    {
        vt->buffer[vt->cursor_row * VTERM_WIDTH + vt->cursor_col] = vga_entry(c, vt->color);
        if (++vt->cursor_col >= VTERM_WIDTH)
        {
            vt->cursor_col = 0;
            if (++vt->cursor_row >= VTERM_SCROLLBACK)
            {
                vterm_scroll(vt);
            }
        }
        if (vt->total_lines < vt->cursor_row + 1)
        {
            vt->total_lines = vt->cursor_row + 1;
        }
    }

    if (do_refresh && vt->active)
    {
        vterm_refresh();
    }
}

void vterm_putchar(struct vterm* vt, const char c)
{
    serial_write(c);
    vterm_putchar_internal(vt, c, true);
}

void vterm_write(struct vterm* vt, const char* str)
{
    if (vt == NULL || str == NULL)
    {
        return;
    }

    while (*str)
    {
        serial_write(*str);
        vterm_putchar_internal(vt, *str++, false);
    }

    if (vt->active)
    {
        vterm_refresh();
    }
}

void vterm_write_dec(struct vterm* vt, uint32_t val)
{
    if (vt == NULL)
    {
        return;
    }

    char buf[12];
    char* ptr = buf + 11;
    *ptr = '\0';

    if (val == 0)
    {
        vterm_putchar(vt, '0');
        return;
    }

    while (val > 0)
    {
        *--ptr = '0' + (val % 10);
        val /= 10;
    }

    vterm_write(vt, ptr);
}

void vterm_clear(struct vterm* vt)
{
    if (vt == NULL)
    {
        return;
    }

    for (int i = 0; i < VTERM_BUFFER_SIZE; i++)
    {
        vt->buffer[i] = vga_entry(' ', vt->color);
    }

    vt->cursor_row = 0;
    vt->cursor_col = 0;
    vt->scroll_offset = 0;
    vt->total_lines = 0;

    if (vt->active)
    {
        vterm_refresh();
    }
}

void vterm_set_color(struct vterm* vt, const uint8_t fg, const uint8_t bg)
{
    if (vt == NULL)
    {
        return;
    }

    vt->color = vga_entry_color(fg, bg);
}

void vterm_set_owner(const uint8_t id, const pid_t pid)
{
    if (id >= VTERM_MAX_COUNT)
    {
        return;
    }

    terminals[id].owner_pid = pid;
}

int vterm_get_by_pid(const pid_t pid)
{
    for (int i = 0; i < VTERM_MAX_COUNT; i++)
    {
        if (terminals[i].owner_pid == pid)
        {
            return i;
        }
    }
    return -1;
}

void vterm_refresh(void)
{
    struct vterm* vt = &terminals[active_terminal];

    uint32_t view_start;
    if (vt->scroll_offset == 0)
    {
        if (vt->cursor_row >= VTERM_HEIGHT)
        {
            view_start = (vt->cursor_row - VTERM_HEIGHT + 1) * VTERM_WIDTH;
        }
        else
        {
            view_start = 0;
        }
    }
    else
    {
        uint32_t current_view_end = vt->cursor_row + 1;
        if (current_view_end < VTERM_HEIGHT)
        {
            current_view_end = VTERM_HEIGHT;
        }
        const uint32_t max_offset = current_view_end > VTERM_HEIGHT ? current_view_end - VTERM_HEIGHT : 0;
        if (vt->scroll_offset > max_offset)
        {
            vt->scroll_offset = max_offset;
        }
        view_start = (current_view_end - VTERM_HEIGHT - vt->scroll_offset) * VTERM_WIDTH;
    }

    memcpy(vga_buffer, vt->buffer + view_start, VTERM_VISIBLE_SIZE * sizeof(uint16_t));

    if (vt->scroll_offset == 0)
    {
        uint32_t visible_row = vt->cursor_row;
        if (visible_row >= VTERM_HEIGHT)
        {
            visible_row = VTERM_HEIGHT - 1;
        }
        const uint16_t cursor_pos = visible_row * VTERM_WIDTH + vt->cursor_col;
        outb(0x3D4, 0x0F);
        outb(0x3D5, (uint8_t)(cursor_pos & 0xFF));
        outb(0x3D4, 0x0E);
        outb(0x3D5, (uint8_t)((cursor_pos >> 8) & 0xFF));
    }
    else
    {
        outb(0x3D4, 0x0F);
        outb(0x3D5, 0xFF);
        outb(0x3D4, 0x0E);
        outb(0x3D5, 0xFF);
    }
}

void vterm_scroll_up(struct vterm* vt, const uint32_t lines)
{
    if (vt == NULL)
    {
        return;
    }

    uint32_t current_view_end = vt->cursor_row + 1;
    if (current_view_end < VTERM_HEIGHT)
    {
        current_view_end = VTERM_HEIGHT;
    }
    const uint32_t max_offset = current_view_end > VTERM_HEIGHT ? current_view_end - VTERM_HEIGHT : 0;

    vt->scroll_offset += lines;
    if (vt->scroll_offset > max_offset)
    {
        vt->scroll_offset = max_offset;
    }

    if (vt->active)
    {
        vterm_refresh();
    }
}

void vterm_scroll_down(struct vterm* vt, const uint32_t lines)
{
    if (vt == NULL)
    {
        return;
    }

    if (vt->scroll_offset >= lines)
    {
        vt->scroll_offset -= lines;
    }
    else
    {
        vt->scroll_offset = 0;
    }

    if (vt->active)
    {
        vterm_refresh();
    }
}

void vterm_scroll_reset(struct vterm* vt)
{
    if (vt == NULL)
    {
        return;
    }

    vt->scroll_offset = 0;

    if (vt->active)
    {
        vterm_refresh();
    }
}

static uint8_t alt_pressed = 0;

#define SCANCODE_PAGEUP   0x49
#define SCANCODE_PAGEDOWN 0x51
#define SCANCODE_HOME     0x47
#define SCANCODE_END      0x4F

bool vterm_handle_switch(const uint8_t scancode)
{
    if (scancode == 0x38)
    {
        alt_pressed = 1;
        return false;
    }

    if (scancode == 0xB8)
    {
        alt_pressed = 0;
        return false;
    }

    struct vterm* vt = vterm_get_active();

    if (scancode == SCANCODE_PAGEUP)
    {
        vterm_scroll_up(vt, VTERM_HEIGHT - 1);
        return true;
    }

    if (scancode == SCANCODE_PAGEDOWN)
    {
        vterm_scroll_down(vt, VTERM_HEIGHT - 1);
        return true;
    }

    if (scancode == SCANCODE_HOME && alt_pressed)
    {
        if (vt)
        {
            uint32_t current_view_end = vt->cursor_row + 1;
            if (current_view_end < VTERM_HEIGHT)
            {
                current_view_end = VTERM_HEIGHT;
            }
            vt->scroll_offset = current_view_end > VTERM_HEIGHT ? current_view_end - VTERM_HEIGHT : 0;
            vterm_refresh();
        }
        return true;
    }

    if (scancode == SCANCODE_END && alt_pressed)
    {
        vterm_scroll_reset(vt);
        return true;
    }

    if (!alt_pressed)
    {
        return false;
    }

    uint8_t new_term = 0xFF;

    switch (scancode)
    {
        case 0x3B: new_term = 0; break;  // F1
        case 0x3C: new_term = 1; break;  // F2
        case 0x3D: new_term = 2; break;  // F3
        case 0x3E: new_term = 3; break;  // F4
        default: return false;
    }

    if (new_term < VTERM_MAX_COUNT && new_term != active_terminal)
    {
        log_info("Switching to terminal");
        vterm_switch(new_term);
        return true;
    }

    return false;
}
