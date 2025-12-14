#include "protocol.h"
#include "console.h"
#include "vterm.h"
#include "../lib/ipc_client.h"
#include "../lib/memory.h"
#include "../lib/io_port.h"

/** Server heap memory */
static uint8_t server_heap[32768] __attribute__((aligned(4096)));

/** Server port ID */
static int server_port = -1;

/** Active virtual terminal */
static int active_vterm = 0;

/** Virtual terminals */
static struct console_vterm vterms[CONSOLE_MAX_VTERMS];

/**
 * @brief Update hardware cursor position
 * @param x Column position
 * @param y Row position
 */
static void update_cursor(uint16_t x, uint16_t y)
{
    uint16_t pos = y * CONSOLE_VGA_WIDTH + x;

    io_outb(0x3D4, 0x0F);
    io_outb(0x3D5, (uint8_t)(pos & 0xFF));
    io_outb(0x3D4, 0x0E);
    io_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/**
 * @brief Handle write request
 * @param msg Incoming message
 */
static void handle_write(struct message *msg)
{
    struct console_write_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct console_vterm *vt = &vterms[active_vterm];

    for (uint8_t i = 0; i < req.length; i++)
    {
        char c = req.data[i];

        if (c == '\n')
        {
            vt->cursor_x = 0;
            vt->cursor_y++;
        }
        else if (c == '\r')
        {
            vt->cursor_x = 0;
        }
        else if (c == '\b')
        {
            if (vt->cursor_x > 0)
            {
                vt->cursor_x--;
            }
        }
        else
        {
            uint32_t offset = (vt->cursor_y * CONSOLE_VGA_WIDTH + vt->cursor_x) * 2;
            if (offset < CONSOLE_VTERM_BUFFER_SIZE - 1)
            {
                vt->buffer[offset] = (uint8_t)c;
                vt->buffer[offset + 1] = (uint8_t)((vt->bg_color << 4) | vt->fg_color);
                vt->cursor_x++;
            }
        }

        if (vt->cursor_x >= CONSOLE_VGA_WIDTH)
        {
            vt->cursor_x = 0;
            vt->cursor_y++;
        }

        if (vt->cursor_y >= CONSOLE_VGA_HEIGHT)
        {
            for (uint32_t row = 0; row < CONSOLE_VGA_HEIGHT - 1; row++)
            {
                uint32_t dst = row * CONSOLE_VGA_WIDTH * 2;
                uint32_t src = (row + 1) * CONSOLE_VGA_WIDTH * 2;
                mem_copy(&vt->buffer[dst], &vt->buffer[src], CONSOLE_VGA_WIDTH * 2);
            }

            uint32_t last_row = (CONSOLE_VGA_HEIGHT - 1) * CONSOLE_VGA_WIDTH * 2;
            for (uint32_t col = 0; col < CONSOLE_VGA_WIDTH; col++)
            {
                vt->buffer[last_row + col * 2] = ' ';
                vt->buffer[last_row + col * 2 + 1] = (uint8_t)((vt->bg_color << 4) | vt->fg_color);
            }

            vt->cursor_y = CONSOLE_VGA_HEIGHT - 1;
        }
    }

    if (vt->active)
    {
        volatile uint8_t *vga = (volatile uint8_t *)CONSOLE_VGA_MEMORY;
        mem_copy((void *)vga, vt->buffer, CONSOLE_VTERM_BUFFER_SIZE);
        update_cursor(vt->cursor_x, vt->cursor_y);
    }

    struct console_response resp = { .status = 0 };
    msg->type = CONSOLE_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle clear request
 * @param msg Incoming message
 */
static void handle_clear(struct message *msg)
{
    struct console_vterm *vt = &vterms[active_vterm];

    for (uint32_t i = 0; i < CONSOLE_VTERM_BUFFER_SIZE; i += 2)
    {
        vt->buffer[i] = ' ';
        vt->buffer[i + 1] = (uint8_t)((vt->bg_color << 4) | vt->fg_color);
    }

    vt->cursor_x = 0;
    vt->cursor_y = 0;

    if (vt->active)
    {
        volatile uint8_t *vga = (volatile uint8_t *)CONSOLE_VGA_MEMORY;
        mem_copy((void *)vga, vt->buffer, CONSOLE_VTERM_BUFFER_SIZE);
        update_cursor(0, 0);
    }

    struct console_response resp = { .status = 0 };
    msg->type = CONSOLE_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle set color request
 * @param msg Incoming message
 */
static void handle_set_color(struct message *msg)
{
    struct console_set_color_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct console_vterm *vt = &vterms[active_vterm];
    vt->fg_color = req.foreground;
    vt->bg_color = req.background;

    struct console_response resp = { .status = 0 };
    msg->type = CONSOLE_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle get size request
 * @param msg Incoming message
 */
static void handle_get_size(struct message *msg)
{
    struct console_size_response resp = {
            .status = 0,
            .width = CONSOLE_VGA_WIDTH,
            .height = CONSOLE_VGA_HEIGHT
    };

    msg->type = CONSOLE_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Process a message from a client
 * @param msg The received message
 */
static void process_message(struct message *msg)
{
    switch (msg->type)
    {
        case CONSOLE_MSG_WRITE:
            handle_write(msg);
            break;
        case CONSOLE_MSG_CLEAR:
            handle_clear(msg);
            break;
        case CONSOLE_MSG_SET_COLOR:
            handle_set_color(msg);
            break;
        case CONSOLE_MSG_GET_SIZE:
            handle_get_size(msg);
            break;
        default:
        {
            struct console_response resp = { .status = -1 };
            msg->type = CONSOLE_MSG_RESPONSE;
            ipc_msg_set_data(msg, &resp, sizeof(resp));
            ipc_reply(msg);
            break;
        }
    }
}

/**
 * @brief Initialize virtual terminals
 */
static void init_vterms(void)
{
    for (int i = 0; i < CONSOLE_MAX_VTERMS; i++)
    {
        vterms[i].id = (uint8_t)i;
        vterms[i].active = (i == 0) ? 1 : 0;
        vterms[i].fg_color = CONSOLE_COLOR_LIGHT_GREY;
        vterms[i].bg_color = CONSOLE_COLOR_BLACK;
        vterms[i].cursor_x = 0;
        vterms[i].cursor_y = 0;
        vterms[i].owner_pid = 0;

        for (uint32_t j = 0; j < CONSOLE_VTERM_BUFFER_SIZE; j += 2)
        {
            vterms[i].buffer[j] = ' ';
            vterms[i].buffer[j + 1] = (uint8_t)((vterms[i].bg_color << 4) | vterms[i].fg_color);
        }
    }
}

/**
 * @brief Console server main function
 * @return Does not return
 */
int main(void)
{
    mem_init(server_heap, sizeof(server_heap));

    ipc_client_init();

    server_port = port_create();
    if (server_port < 0)
    {
        return -1;
    }

    ipc_register_server(CONSOLE_SERVER_PORT_NAME, server_port);

    init_vterms();

    struct message msg;
    while (1)
    {
        int ret = ipc_receive(server_port, &msg, true);
        if (ret == IPC_SUCCESS)
        {
            process_message(&msg);
        }
    }

    return 0;
}
