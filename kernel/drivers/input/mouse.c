#include "mouse.h"
#include "arch/i686/arch.h"
#include "arch/i686/idt.h"
#include "drivers/video/vesa.h"
#include "ui/console.h"
#include "lib/string.h"
#include "cast.h"

#define MOUSE_ACK 0xFA

static int32_t cursor_x = 0;
static int32_t cursor_y = 0;
static uint8_t button_state = 0;
static volatile uint32_t state_dirty = 0;

static uint8_t packet[3];
static uint8_t packet_index = 0;

static void mouse_wait_write(void)
{
    for (int timeout = 100000; timeout > 0; timeout--)
    {
        if ((inb(MOUSE_STATUS_PORT) & 0x02) == 0)
        {
            return;
        }
    }
}

static void mouse_wait_read(void)
{
    for (int timeout = 100000; timeout > 0; timeout--)
    {
        if (inb(MOUSE_STATUS_PORT) & 0x01)
        {
            return;
        }
    }
}

static void mouse_write_cmd(const uint8_t cmd)
{
    mouse_wait_read();
    outb(MOUSE_STATUS_PORT, 0xD4);
    mouse_wait_read();
    outb(MOUSE_DATA_PORT, cmd);
}

static uint8_t mouse_read_data(void)
{
    mouse_wait_read();
    return inb(MOUSE_DATA_PORT);
}

static void clamp_cursor(void)
{
    const int32_t max_x = vesa_get_width() > 0 ? (int32_t)vesa_get_width() - 1 : 0;
    const int32_t max_y = vesa_get_height() > 0 ? (int32_t)vesa_get_height() - 1 : 0;

    if (cursor_x < 0) cursor_x = 0;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_x > max_x) cursor_x = max_x;
    if (cursor_y > max_y) cursor_y = max_y;
}

static void mouse_callback(struct registers* regs)
{
    UNUSED(regs, "use registers in future");
    const uint8_t data = inb(MOUSE_DATA_PORT);

    if (packet_index == 0 && (data & 0x08) == 0)
    {
        return;
    }

    packet[packet_index++] = data;
    if (packet_index < 3)
    {
        return;
    }

    packet_index = 0;

    const uint8_t flags = packet[0];

    if (flags & 0xC0)
    {
        return;
    }

    int16_t dx = packet[1];
    int16_t dy = packet[2];

    if (flags & 0x10) dx = (int16_t)(dx - 256);
    if (flags & 0x20) dy = (int16_t)(dy - 256);

    cursor_x += dx;
    cursor_y += dy;
    clamp_cursor();

    const uint8_t new_buttons = flags & 0x07;
    if (new_buttons != button_state || dx != 0 || dy != 0)
    {
        state_dirty = 1;
    }

    button_state = new_buttons;
}

void mouse_init(void)
{
    outb(MOUSE_STATUS_PORT, 0xAD);
    outb(MOUSE_STATUS_PORT, 0xA7);

    if (inb(MOUSE_STATUS_PORT) & 0x01)
    {
        inb(MOUSE_DATA_PORT);
    }
    else
    {
        log_error_fmt("%s: mouse data port had pending data", __FUNCTION__);
    }

    outb(MOUSE_STATUS_PORT, 0xA8);
    outb(MOUSE_STATUS_PORT, 0xAE);

    outb(MOUSE_STATUS_PORT, 0x20);
    uint8_t config = mouse_read_data();
    config |= 0x02;
    config |= 0x01;
    config &= (uint8_t)~0x20;
    config &= (uint8_t)~0x10;

    outb(MOUSE_STATUS_PORT, 0x60);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, config);

    mouse_write_cmd(0xF6);
    mouse_read_data(); // ACK

    mouse_write_cmd(0xF4);
    mouse_read_data(); // ACK

    cursor_x = (int32_t)vesa_get_width() / 2;
    cursor_y = (int32_t)vesa_get_height() / 2;
    button_state = 0;
    state_dirty = 1;
    packet_index = 0;

    register_interrupt_handler(44, mouse_callback);
}

int mouse_try_get_state(struct mouse_state* state)
{
    if (!state)
    {
        return 0;
    }

    if (state_dirty)
    {
        state->x = cursor_x;
        state->y = cursor_y;
        state->buttons = button_state;
        state->moved = 1;
        state_dirty = 0;
    }
    else
    {
        state->x = cursor_x;
        state->y = cursor_y;
        state->buttons = button_state;
        state->moved = 0;
    }

    return 1;
}

void mouse_shutdown(void)
{
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: mouse driver shutdown complete\n", __FUNCTION__);
    console_write(msg);
}