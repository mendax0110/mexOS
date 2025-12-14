#include "protocol.h"
#include "keyboard.h"
#include "../lib/ipc_client.h"
#include "../lib/memory.h"
#include "../lib/io_port.h"

/** Server heap memory */
static uint8_t server_heap[16384] __attribute__((aligned(4096)));

/** Server port ID */
static int server_port = -1;

/** Registered input clients */
static struct input_client clients[INPUT_MAX_CLIENTS];

/** Current input state */
static struct input_state kb_state;

/** Event queue */
static struct input_event event_queue[INPUT_QUEUE_SIZE];
static uint32_t queue_head = 0;
static uint32_t queue_tail = 0;

/** US keyboard scancode to ASCII table (unshifted) */
static const char scancode_to_ascii[128] =
        {
                0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
                '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
                0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
                0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
                '*', 0, ' ', 0
        };

/** US keyboard scancode to ASCII table (shifted) */
static const char scancode_to_ascii_shift[128] =
        {
                0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
                '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
                0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
                0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
                '*', 0, ' ', 0
        };

/**
 * @brief Add event to queue
 * @param event Event to add
 */
static void queue_event(const struct input_event *event)
{
    uint32_t next_tail = (queue_tail + 1) % INPUT_QUEUE_SIZE;
    if (next_tail != queue_head)
    {
        event_queue[queue_tail] = *event;
        queue_tail = next_tail;
    }
}

/**
 * @brief Notify registered clients of an event
 * @param event Event to notify
 */
static void notify_clients(const struct input_event *event)
{
    for (int i = 0; i < INPUT_MAX_CLIENTS; i++)
    {
        if (clients[i].active && (clients[i].event_mask & (1U << event->type)))
        {
            struct message msg;
            ipc_msg_init(&msg, INPUT_MSG_EVENT);
            ipc_msg_set_data(&msg, event, sizeof(*event));
            ipc_send_async(clients[i].port_id, &msg);
        }
    }
}

/**
 * @brief Process keyboard scancode
 * @param scancode Raw keyboard scancode
 */
static void process_scancode(uint8_t scancode)
{
    struct input_event event;
    mem_set(&event, 0, sizeof(event));

    bool released = (scancode & 0x80) != 0;
    scancode &= 0x7F;

    switch (scancode)
    {
        case 0x2A: /* Left Shift */
        case 0x36: /* Right Shift */
            kb_state.shift_pressed = released ? 0 : 1;
            return;
        case 0x1D: /* Left Ctrl */
            kb_state.ctrl_pressed = released ? 0 : 1;
            return;
        case 0x38: /* Left Alt */
            kb_state.alt_pressed = released ? 0 : 1;
            return;
        case 0x3A: /* Caps Lock */
            if (!released)
            {
                kb_state.caps_lock = !kb_state.caps_lock;
            }
            return;
    }

    event.type = released ? INPUT_EVENT_KEY_RELEASE : INPUT_EVENT_KEY_PRESS;
    event.scancode = scancode;
    event.modifiers = 0;

    if (kb_state.shift_pressed)
    {
        event.modifiers |= INPUT_MOD_SHIFT;
    }
    if (kb_state.ctrl_pressed)
    {
        event.modifiers |= INPUT_MOD_CTRL;
    }
    if (kb_state.alt_pressed)
    {
        event.modifiers |= INPUT_MOD_ALT;
    }
    if (kb_state.caps_lock)
    {
        event.modifiers |= INPUT_MOD_CAPS;
    }

    if (scancode < 128)
    {
        bool shifted = kb_state.shift_pressed;

        if (kb_state.caps_lock && scancode >= 0x10 && scancode <= 0x32)
        {
            shifted = !shifted;
        }

        event.keychar = shifted ? scancode_to_ascii_shift[scancode]
                                : scancode_to_ascii[scancode];
    }

    queue_event(&event);
    notify_clients(&event);
}

/**
 * @brief Handle register request
 * @param msg Incoming message
 */
static void handle_register(struct message *msg)
{
    struct input_register_request req;
    ipc_msg_get_data(msg, &req, sizeof(req));

    struct input_response resp = { .status = -1 };

    /* Find free slot */
    for (int i = 0; i < INPUT_MAX_CLIENTS; i++)
    {
        if (!clients[i].active)
        {
            clients[i].pid = msg->sender;
            clients[i].port_id = req.port_id;
            clients[i].event_mask = req.event_mask;
            clients[i].active = 1;
            resp.status = 0;
            break;
        }
    }

    msg->type = INPUT_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle unregister request
 * @param msg Incoming message
 */
static void handle_unregister(struct message *msg)
{
    struct input_response resp = { .status = -1 };

    for (int i = 0; i < INPUT_MAX_CLIENTS; i++)
    {
        if (clients[i].active && clients[i].pid == msg->sender)
        {
            clients[i].active = 0;
            resp.status = 0;
            break;
        }
    }

    msg->type = INPUT_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle poll request
 * @param msg Incoming message
 */
static void handle_poll(struct message *msg)
{
    uint32_t pending = 0;
    if (queue_head != queue_tail)
    {
        if (queue_tail >= queue_head)
        {
            pending = queue_tail - queue_head;
        }
        else
        {
            pending = INPUT_QUEUE_SIZE - queue_head + queue_tail;
        }
    }

    struct input_poll_response resp = {
            .status = 0,
            .events_pending = pending
    };

    msg->type = INPUT_MSG_RESPONSE;
    ipc_msg_set_data(msg, &resp, sizeof(resp));
    ipc_reply(msg);
}

/**
 * @brief Handle read request
 * @param msg Incoming message
 */
static void handle_read(struct message *msg)
{
    struct input_read_response resp;
    mem_set(&resp, 0, sizeof(resp));
    resp.status = 0;
    resp.event_count = 0;

    while (queue_head != queue_tail && resp.event_count < INPUT_MAX_EVENTS)
    {
        resp.events[resp.event_count++] = event_queue[queue_head];
        queue_head = (queue_head + 1) % INPUT_QUEUE_SIZE;
    }

    msg->type = INPUT_MSG_RESPONSE;
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
        case INPUT_MSG_REGISTER:
            handle_register(msg);
            break;
        case INPUT_MSG_UNREGISTER:
            handle_unregister(msg);
            break;
        case INPUT_MSG_POLL:
            handle_poll(msg);
            break;
        case INPUT_MSG_READ:
            handle_read(msg);
            break;
        default:
        {
            struct input_response resp = { .status = -1 };
            msg->type = INPUT_MSG_RESPONSE;
            ipc_msg_set_data(msg, &resp, sizeof(resp));
            ipc_reply(msg);
            break;
        }
    }
}

/**
 * @brief Poll keyboard for input
 */
static void poll_keyboard(void)
{
    uint8_t status = io_inb(INPUT_KB_STATUS_PORT);

    if (status & INPUT_KB_STATUS_OUTPUT_FULL)
    {
        uint8_t scancode = io_inb(INPUT_KB_DATA_PORT);
        process_scancode(scancode);
    }
}

/**
 * @brief Input server main function
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

    ipc_register_server(INPUT_SERVER_PORT_NAME, server_port);

    mem_set(&kb_state, 0, sizeof(kb_state));
    mem_set(clients, 0, sizeof(clients));

    struct message msg;
    while (1)
    {
        poll_keyboard();

        int ret = ipc_receive(server_port, &msg, false);
        if (ret == IPC_SUCCESS)
        {
            process_message(&msg);
        }

        for (volatile int i = 0; i < 1000; i++);
    }

    return 0;
}