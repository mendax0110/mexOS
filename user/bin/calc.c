#include "../shared/math.h"
#include "../shared/display_abi.h"
#include "runtime.h"
#include "gfx.h"

#define CALC_WIDTH 220
#define CALC_HEIGHT 300

enum calc_op
{
    OP_DIV,
    OP_MUL,
    OP_SUB,
    OP_ADD
};

struct calc_button
{
    int x, y, w, h;
    const char* label;
};

static struct
{
    int32_t display_value;
    int32_t pending;
    enum calc_op pending_op;
    bool has_pending;
    bool entering_new;
} calc_state = { 0, 0, OP_ADD, false, true };

static struct vesa_mode_info surface_mode;
static uint8_t* surface;
static uint32_t window_id;
static int event_port;

static const struct calc_button buttons[] = {
    { 12,  60, 44, 44, "7" }, { 60,  60, 44, 44, "8" }, { 108, 60, 44, 44, "9" }, { 156, 60, 44, 44, "/" },
{ 12, 108, 44, 44, "4" }, { 60, 108, 44, 44, "5" }, { 108,108, 44, 44, "6" }, { 156,108, 44, 44, "X" },
{ 12, 156, 44, 44, "1" }, { 60, 156, 44, 44, "2" }, { 108,156, 44, 44, "3" }, { 156,156, 44, 44, "-" },
{ 12, 204, 44, 44, "C" }, { 60, 204, 44, 44, "0" }, { 108,204, 44, 44,"EQ" }, { 156,204, 44, 44, "+" },
};

#define BUTTON_COUNT (sizeof(buttons) / sizeof(buttons[0]))

static void int_to_str(int32_t num, char* out)
{
    if (num == 0)
    {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    bool is_negative = false;
    if (num < 0)
    {
        is_negative = true;
        num = -num;
    }

    char buffer[12];
    int i = 0;
    while (num > 0)
    {
        buffer[i++] = (char)('0' + (num % 10));
        num /= 10;
    }

    if (is_negative)
    {
        buffer[i++] = '-';
    }

    for (int j = 0; j < i; j++)
    {
        out[j] = buffer[i - j - 1];
    }
    out[i] = '\0';
}

static const char* op_symbol(const enum calc_op op)
{
    switch (op)
    {
        case OP_DIV: return "/";
        case OP_MUL: return "X";
        case OP_SUB: return "-";
        case OP_ADD: return "+";
        default:     return "";
    }
}

static int32_t do_division(const int32_t a, const int32_t b)
{
    if (b == 0)
    {
        user_println("Error: Division by zero");
        return 0;
    }
    return a / b;
}

static int32_t do_multiplication(const int32_t a, const int32_t b)
{
    return a * b;
}

static int32_t do_subtraction(const int32_t a, const int32_t b)
{
    return a - b;
}

static int32_t do_addition(const int32_t a, const int32_t b)
{
    return a + b;
}

static int32_t apply_op(const int32_t a, const int32_t b, const enum calc_op op)
{
    switch (op)
    {
        case OP_DIV: return do_division(a, b);
        case OP_MUL: return do_multiplication(a, b);
        case OP_SUB: return do_subtraction(a, b);
        case OP_ADD: return do_addition(a, b);
        default: return 0;
    }
}

static void draw_calculator(void)
{
    const uint32_t bg     = gfx_rgb(&surface_mode, 8, 15, 28);
    const uint32_t screen = gfx_rgb(&surface_mode, 15, 23, 42);
    const uint32_t key    = gfx_rgb(&surface_mode, 51, 65, 85);
    const uint32_t key_op = gfx_rgb(&surface_mode, 14, 165, 233);
    const uint32_t white  = gfx_rgb(&surface_mode, 241, 245, 249);
    const uint32_t muted  = gfx_rgb(&surface_mode, 148, 163, 184);

    gfx_rect(&surface_mode, surface, 0, 0, CALC_WIDTH, CALC_HEIGHT, bg);
    gfx_rect(&surface_mode, surface, 12, 12, CALC_WIDTH - 24, 36, screen);

    if (calc_state.has_pending)
    {
        char pending_text[16];
        int_to_str(calc_state.pending, pending_text);

        char expr[24];
        size_t p = 0;
        for (size_t i = 0; pending_text[i] && p < sizeof(expr) - 4; i++)
        {
            expr[p++] = pending_text[i];
        }
        expr[p++] = ' ';
        const char* sym = op_symbol(calc_state.pending_op);
        for (size_t i = 0; sym[i] && p < sizeof(expr) - 1; i++)
        {
            expr[p++] = sym[i];
        }
        expr[p] = '\0';

        gfx_text(&surface_mode, surface, expr, 18, 16, 1, muted);
    }

    char text[16];
    int_to_str(calc_state.display_value, text);
    gfx_text(&surface_mode, surface, text, CALC_WIDTH - 20 - (int)user_strlen(text) * 6, 28, 1, white);

    for (size_t i = 0; i < BUTTON_COUNT; i++)
    {
        const char c = buttons[i].label[0];
        const bool is_op = c == '/' || c == 'X' || c == '-' || c == '+' || c == 'E' || c == 'C';
        gfx_rect(&surface_mode, surface, buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h, is_op ? key_op : key);
        gfx_text(&surface_mode, surface, buttons[i].label, buttons[i].x + buttons[i].w / 2 - (int)user_strlen(buttons[i].label) * 3, buttons[i].y + buttons[i].h / 2 - 4, 1, white);
    }
}

static void send_packet_to_server(const struct display_packet* packet)
{
    struct message message;
    user_memset(&message, 0, sizeof(message));
    message.sender = getpid();
    message.type = packet->type;
    message.len = sizeof(*packet);
    user_memcpy(message.data, packet, sizeof(*packet));
    send(DISPLAY_SERVER_PORT, &message, IPC_NONBLOCK);
}

static void notify_dirty(void)
{
    struct display_packet commit;
    user_memset(&commit, 0, sizeof(commit));
    commit.type = DISPLAY_COMMIT_SURFACE;
    commit.window_id = window_id;
    send_packet_to_server(&commit);
}

static void handle_button(const char* label)
{
    const char c = label[0];

    if (c >= '0' && c <= '9')
    {
        const int32_t digit = c - '0';
        calc_state.display_value = calc_state.entering_new
                                    ? digit
                                    : calc_state.display_value * 10 + digit;
        calc_state.entering_new = false;
        return;
    }

    if (c == 'C')
    {
        calc_state.display_value = 0;
        calc_state.pending = 0;
        calc_state.has_pending = false;
        calc_state.entering_new = true;
        return;
    }

    if (c == 'E')
    {
        if (calc_state.has_pending)
        {
            calc_state.display_value = apply_op(calc_state.pending, calc_state.display_value, calc_state.pending_op);
            calc_state.has_pending = false;
        }
        calc_state.entering_new = true;
        return;
    }

    enum calc_op op;

    switch (c)
    {
        case '/': op = OP_DIV; break;
        case 'X': op = OP_MUL; break;
        case '-': op = OP_SUB; break;
        case '+': op = OP_ADD; break;
        default: return;
    }

    if (calc_state.has_pending)
    {
        calc_state.display_value = apply_op(calc_state.pending, calc_state.display_value, calc_state.pending_op);
    }
    calc_state.pending = calc_state.display_value;
    calc_state.pending_op = op;
    calc_state.has_pending = true;
    calc_state.entering_new = true;
}

static void handle_click(const int x, const int y)
{
    for (size_t i = 0; i < BUTTON_COUNT; i++)
    {
        if (x >= buttons[i].x && x < buttons[i].x + buttons[i].w &&
            y >= buttons[i].y && y < buttons[i].y + buttons[i].h)
        {
            handle_button(buttons[i].label);
            draw_calculator();
            notify_dirty();
            return;
        }
    }
}

static void handle_key(const unsigned char key)
{
    if (key >= '0' && key <= '9')
    {
        char digit[2] = { (char)key, '\0' };
        handle_button(digit);
    }
    else if (key == '+')
    {
        handle_button("+");
    }
    else if (key == '-')
    {
        handle_button("-");
    }
    else if (key == '*' || key == 'x' || key == 'X')
    {
        handle_button("X");
    }
    else if (key == '/')
    {
        handle_button("/");
    }
    else if (key == '=' || key == '\n' || key == '\r')
    {
        handle_button("EQ");
    }
    else if (key == 'c' || key == 'C' || key == 27)
    {
        handle_button("C");
    }
    else
    {
        return;
    }

    draw_calculator();
    notify_dirty();
}

int main(void)
{
    event_port = port_create();
    if (event_port < 0) return 1;

    const int shm_id = shm_create(CALC_WIDTH * CALC_HEIGHT * 4);
    if (shm_id < 0) return 1;
    surface = shm_map(shm_id);
    if (!surface) return 1;

    surface_mode.width = CALC_WIDTH;
    surface_mode.height = CALC_HEIGHT;
    surface_mode.bpp = 32;
    surface_mode.pitch = CALC_WIDTH * 4;
    surface_mode.red_size = 8;
    surface_mode.red_pos = 16;
    surface_mode.green_size = 8;
    surface_mode.green_pos = 8;
    surface_mode.blue_size = 8;
    surface_mode.blue_pos = 0;

    struct display_packet create;
    user_memset(&create, 0, sizeof(create));
    create.type = DISPLAY_CREATE_WINDOW;
    create.event_port = event_port;
    create.a = shm_id;
    create.c = CALC_WIDTH;
    create.d = CALC_HEIGHT;
    const char* title = "CALCULATOR";
    for (size_t i = 0; title[i] && i < sizeof(create.text) - 1; i++)
    {
        create.text[i] = title[i];
    }

    struct message msg;
    user_memset(&msg, 0, sizeof(msg));
    msg.sender = getpid();
    msg.type = create.type;
    msg.len = sizeof(create);
    user_memcpy(msg.data, &create, sizeof(create));
    send(DISPLAY_SERVER_PORT, &msg, IPC_NONBLOCK);

    draw_calculator();

    bool created = false;
    while (1)
    {
        struct message m;
        while (recv(event_port, &m, IPC_NONBLOCK) == 0)
        {
            const struct display_packet* event = (const struct display_packet*)m.data;

            if (event->type == DISPLAY_EVENT_CREATED && !created)
            {
                window_id = event->window_id;
                created = true;
            }
            else if (event->type == DISPLAY_EVENT_CLICK)
            {
                handle_click(event->a, event->b);
            }
            else if (event->type == DISPLAY_EVENT_KEY)
            {
                handle_key((unsigned char)event->a);
            }
            else if (event->type == DISPLAY_EVENT_CLOSE)
            {
                shm_detach(shm_id);
                shm_destroy(shm_id);
                return 0;
            }
        }

        yield();
    }
}