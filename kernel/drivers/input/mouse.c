#include "mouse.h"
#include "arch/i686/arch.h"
#include "arch/i686/idt.h"
#include "drivers/video/vesa.h"
#include "ui/console.h"
#include "lib/string.h"
#include "lib/log.h"

#define MOUSE_ACK 0xFA
#define MOUSE_RESEND 0xFE
#define MOUSE_STATUS_OUTPUT_FULL 0x01
#define MOUSE_STATUS_INPUT_FULL  0x02
#define MOUSE_STATUS_AUX_DATA    0x20
#define MOUSE_COMMAND_PORT 0x64
#define MOUSE_TIMEOUT 100000

static int32_t cursor_x = 0;
static int32_t cursor_y = 0;
static uint8_t button_state = 0;
static volatile uint32_t state_dirty = 0;
static bool mouse_available = false;

static uint8_t packet[3];
static uint8_t packet_index = 0;

static bool mouse_wait_write(void)
{
    for (int timeout = MOUSE_TIMEOUT; timeout > 0; timeout--)
    {
        if ((inb(MOUSE_STATUS_PORT) & MOUSE_STATUS_INPUT_FULL) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool mouse_wait_read(void)
{
    for (int timeout = MOUSE_TIMEOUT; timeout > 0; timeout--)
    {
        if (inb(MOUSE_STATUS_PORT) & MOUSE_STATUS_OUTPUT_FULL)
        {
            return true;
        }
    }
    return false;
}

static bool controller_command(const uint8_t command)
{
    if (!mouse_wait_write()) return false;
    outb(MOUSE_COMMAND_PORT, command);
    return true;
}

static bool controller_write_data(const uint8_t data)
{
    if (!mouse_wait_write()) return false;
    outb(MOUSE_DATA_PORT, data);
    return true;
}

static bool mouse_read_data(uint8_t* data)
{
    if (!data || !mouse_wait_read()) return false;
    *data = inb(MOUSE_DATA_PORT);
    return true;
}

static void controller_flush_output(void)
{
    for (int i = 0; i < 32; i++)
    {
        if ((inb(MOUSE_STATUS_PORT) & MOUSE_STATUS_OUTPUT_FULL) == 0) return;
        (void)inb(MOUSE_DATA_PORT);
    }
}

static bool mouse_send(const uint8_t command)
{
    for (int attempt = 0; attempt < 3; attempt++)
    {
        if (!controller_command(0xD4) || !controller_write_data(command))
        {
            return false;
        }

        uint8_t response = 0;
        if (!mouse_read_data(&response))
        {
            return false;
        }
        if (response == MOUSE_ACK)
        {
            return true;
        }
        if (response != MOUSE_RESEND)
        {
            return false;
        }
    }
    return false;
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

static void mouse_process_byte(const uint8_t data)
{
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
    cursor_y -= dy;
    clamp_cursor();

    const uint8_t new_buttons = flags & 0x07;
    if (new_buttons != button_state || dx != 0 || dy != 0)
    {
        state_dirty = 1;
    }

    button_state = new_buttons;
}

static void mouse_poll_controller(void)
{
    for (int i = 0; i < 32; i++)
    {
        const uint8_t status = inb(MOUSE_STATUS_PORT);
        if ((status & MOUSE_STATUS_OUTPUT_FULL) == 0 ||
            (status & MOUSE_STATUS_AUX_DATA) == 0)
        {
            return;
        }
        mouse_process_byte(inb(MOUSE_DATA_PORT));
    }
}

static void mouse_callback(struct registers* regs)
{
    UNUSED(regs, "use registers in future");
    const uint8_t status = inb(MOUSE_STATUS_PORT);
    if ((status & MOUSE_STATUS_OUTPUT_FULL) == 0 ||
        (status & MOUSE_STATUS_AUX_DATA) == 0)
    {
        return;
    }
    mouse_process_byte(inb(MOUSE_DATA_PORT));
}

void mouse_init(void)
{
    mouse_available = false;
    packet_index = 0;

    if (!controller_command(0xAD) || !controller_command(0xA7))
    {
        log_error("PS/2 controller did not accept disable commands");
        return;
    }
    controller_flush_output();

    if (!controller_command(0xA8) || !controller_command(0xAE))
    {
        log_error("PS/2 controller did not enable its ports");
        return;
    }

    uint8_t config = 0;
    if (!controller_command(0x20) || !mouse_read_data(&config))
    {
        log_error("PS/2 controller configuration read failed");
        return;
    }
    config |= 0x02;
    config |= 0x01;
    config &= (uint8_t)~0x20;
    config &= (uint8_t)~0x10;

    if (!controller_command(0x60) || !controller_write_data(config))
    {
        log_error("PS/2 controller configuration write failed");
        return;
    }

    if (!mouse_send(0xF6) || !mouse_send(0xF4))
    {
        log_error("No PS/2 auxiliary pointing device responded");
        return;
    }

    register_interrupt_handler(44, mouse_callback);

    cursor_x = (int32_t)vesa_get_width() / 2;
    cursor_y = (int32_t)vesa_get_height() / 2;
    button_state = 0;
    state_dirty = 1;
    packet_index = 0;
    mouse_available = true;
    log_info("PS/2 pointing device enabled");
}

int mouse_try_get_state(struct mouse_state* state)
{
    if (!state || !mouse_available)
    {
        return 0;
    }

    mouse_poll_controller();

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
    mouse_available = false;
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: mouse driver shutdown complete\n", __FUNCTION__);
    console_write(msg);
}
