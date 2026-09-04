#include "runtime.h"
#include "gfx.h"
#include "../shared/display_abi.h"
#include "arch/i686/arch.h"

#define DISPLAY_MAX_WINDOWS 8
#define DISPLAY_LINES 64
#define DISPLAY_COLUMNS 72
#define PANEL_HEIGHT 46
#define TOPBAR_HEIGHT 42
#define LAUNCHER_X 32
#define LAUNCHER_Y 96
#define LAUNCHER_WIDTH 176
#define LAUNCHER_HEIGHT 104

/**
 * @brief Structure representing a window in the display server. \struct display_window
 */
struct display_window
{
    bool used;
    bool minimized;
    bool maximized;
    uint32_t id;
    int event_port;
    int x;
    int y;
    int width;
    int height;
    uint32_t z;
    char title[64];
    char lines[DISPLAY_LINES][DISPLAY_COLUMNS];
    int line;
    int column;
    int restore_x;
    int restore_y;
    int restore_width;
    int restore_height;
    int shm_id;
    uint8_t* surface;
};

/**
 * @brief Structure representing the state of the display server. \struct display_state
 */
struct display_state
{
    struct vesa_mode_info mode;
    uint8_t* framebuffer;
    uint8_t* backbuffer;
    struct display_window windows[DISPLAY_MAX_WINDOWS];
    uint32_t next_id;
    uint32_t next_z;
    int desktop_port;
    int focused;
    bool start_open;
    int confirm_action;
    bool dragging;
    bool resizing;
    int drag_window;
    int drag_dx;
    int drag_dy;
    bool mouse_down;
    struct mouse_state mouse;
    bool pointer_available;
    struct rtc_time clock;
    bool clock_valid;
    bool heartbeat;
    uint64_t last_frame_ticks;
    const uint64_t frame_interval_ticks;
    bool mouse_position_only;
};

/**
 * @brief Function to check if the compositor heartbeat is active.
 * @return true if the heartbeat is active, false otherwise.
 */
static bool compositor_heartbeat(void)
{
    uint32_t low;
    uint32_t high;
    ASM_V("rdtsc" : "=a"(low), "=d"(high));
    UNUSED(high);
    return (low & (1U << 30)) != 0;
}

/**
 * @brief Draws the boot stage on the display.
 * @param state Pointer to the display state structure.
 * @param stage The current boot stage string to be displayed.
 */
static void draw_boot_stage(const struct display_state* state, const char* stage)
{
    const int width = 104;
    const int x = (int)state->mode.width / 2 - width / 2;
    const uint32_t panel = gfx_rgb(&state->mode, 15, 23, 42);
    const uint32_t accent = gfx_rgb(&state->mode, 251, 191, 36);
    const uint32_t white = gfx_rgb(&state->mode, 241, 245, 249);
    gfx_rect(&state->mode, state->framebuffer, x, 8, width, 25, panel);
    gfx_frame(&state->mode, state->framebuffer, x, 8, width, 25, accent);
    gfx_text(&state->mode, state->framebuffer, "BOOT", x + 8, 17, 1, accent);
    gfx_text(&state->mode, state->framebuffer, stage, x + 44, 17, 1, white);
}

/**
 * @brief Copies a string from source to destination with size limit.
 * @param destination The destination buffer where the string will be copied.
 * @param size The maximum size of the destination buffer.
 * @param source The source string to be copied.
 */
static void string_copy(char* destination, const size_t size, const char* source)
{
    if (!destination || size == 0) return;
    size_t i = 0;
    while (source && source[i] && i + 1 < size)
    {
        destination[i] = source[i];
        i++;
    }
    destination[i] = '\0';
}

/**
 * @brief Formats the current time into a string.
 * @param state Pointer to the display state structure.
 * @param output The buffer to store the formatted time string.
 */
static void clock_text(const struct display_state* state, char output[9])
{
    if (!state->clock_valid)
    {
        string_copy(output, 9, "--:--:--");
        return;
    }

    output[0] = (char)('0' + (state->clock.hour / 10U) % 10U);
    output[1] = (char)('0' + state->clock.hour % 10U);
    output[2] = ':';
    output[3] = (char)('0' + (state->clock.minute / 10U) % 10U);
    output[4] = (char)('0' + state->clock.minute % 10U);
    output[5] = ':';
    output[6] = (char)('0' + (state->clock.second / 10U) % 10U);
    output[7] = (char)('0' + state->clock.second % 10U);
    output[8] = '\0';
}

/**
 * @brief Checks if a point is inside a rectangle.
 * @param px The x-coordinate of the point.
 * @param py The y-coordinate of the point.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @return true if the point is inside the rectangle, false otherwise.
 */
static bool inside(const int px, const int py, const int x, const int y, const int width, const int height)
{
    return px >= x && py >= y && px < x + width && py < y + height;
}

/**
 * @brief Sends a display packet to the specified port.
 * @param port The port to send the packet to.
 * @param packet Pointer to the display packet to be sent.
 * @return 0 on success, -1 on failure.
 */
static int send_packet(const int port, const struct display_packet* packet)
{
    if (port < 0) return -1;
    struct message message;
    user_memset(&message, 0, sizeof(message));
    message.sender = getpid();
    message.type = packet->type;
    message.len = sizeof(*packet);
    user_memcpy(message.data, packet, sizeof(*packet));
    return send(port, &message, IPC_NONBLOCK);
}

/**
 * @brief Finds a window by its ID in the display state.
 * @param state Pointer to the display state structure.
 * @param id The ID of the window to find.
 * @return Pointer to the display window if found, NULL otherwise.
 */
static struct display_window* window_by_id(struct display_state* state, const uint32_t id)
{
    for (int i = 0; i < DISPLAY_MAX_WINDOWS; i++)
    {
        if (state->windows[i].used && state->windows[i].id == id)
        {
            return &state->windows[i];
        }
    }
    return NULL;
}

/**
 * @brief Gets the index of a window in the display state.
 * @param state Pointer to the display state structure.
 * @param window Pointer to the display window structure.
 * @return The index of the window, or -1 if not found.
 */
static int window_index(const struct display_state* state, const struct display_window* window)
{
    return (int)(window - state->windows);
}

/**
 * @brief Focuses on a window in the display state.
 * @param state Pointer to the display state structure.
 * @param index The index of the window to focus on.
 */
static void focus_window(struct display_state* state, const int index)
{
    if (index < 0 || index >= DISPLAY_MAX_WINDOWS || !state->windows[index].used) return;
    state->focused = index;
    state->windows[index].minimized = false;
    state->windows[index].z = ++state->next_z;
}

/**
 * @brief Appends a character to the text buffer of a display window.
 * @param window Pointer to the display window structure.
 * @param character The character to append.
 */
static void append_character(struct display_window* window, const char character)
{
    if (character == '\r') return;
    if (character == '\b')
    {
        if (window->column > 0)
        {
            window->lines[window->line][--window->column] = '\0';
        }
        return;
    }
    if (character == '\n' || window->column + 1 >= DISPLAY_COLUMNS)
    {
        window->line++;
        window->column = 0;
        if (window->line >= DISPLAY_LINES)
        {
            for (int i = 1; i < DISPLAY_LINES; i++)
            {
                string_copy(window->lines[i - 1], DISPLAY_COLUMNS, window->lines[i]);
            }
            window->line = DISPLAY_LINES - 1;
        }
        window->lines[window->line][0] = '\0';
        if (character == '\n') return;
    }
    if (character >= 0x20 && character < 0x7f)
    {
        window->lines[window->line][window->column++] = character;
        window->lines[window->line][window->column] = '\0';
    }
}

/**
 * @brief Handles a display packet.
 * @param state Pointer to the display state structure.
 * @param packet Pointer to the display packet to be handled.
 */
static void handle_packet(struct display_state* state, const struct display_packet* packet)
{
    if (packet->type == DISPLAY_REGISTER_DESKTOP)
    {
        state->desktop_port = packet->event_port;
        return;
    }
    if (packet->type == DISPLAY_CREATE_WINDOW)
    {
        for (int i = 0; i < DISPLAY_MAX_WINDOWS; i++)
        {
            if (state->windows[i].used) continue;
            struct display_window* window = &state->windows[i];
            user_memset(window, 0, sizeof(*window));
            window->used = true;
            window->id = ++state->next_id;
            window->event_port = packet->event_port;
            window->shm_id = packet->a;
            window->width = packet->c > 200 ? packet->c : 620;
            window->height = packet->d > 120 ? packet->d : 390;
            window->x = 90 + (int)(window->id % 4U) * 34;
            window->y = 60 + (int)(window->id % 4U) * 30;
            string_copy(window->title, sizeof(window->title), packet->text[0] ? packet->text : "WINDOW");
            if (window->shm_id >= 0)
            {
                window->surface = shm_map(window->shm_id);
            }
            focus_window(state, i);

            struct display_packet response;
            user_memset(&response, 0, sizeof(response));
            response.type = DISPLAY_EVENT_CREATED;
            response.window_id = window->id;
            response.a = state->mode.width;
            response.b = state->mode.height;
            response.c = state->mode.bpp;
            response.d = state->mode.pitch;
            send_packet(window->event_port, &response);
            return;
        }
        return;
    }

    struct display_window* window = window_by_id(state, packet->window_id);
    if (!window) return;
    if (packet->type == DISPLAY_APPEND_TEXT)
    {
        for (size_t i = 0; packet->text[i] && i < sizeof(packet->text); i++)
        {
            append_character(window, packet->text[i]);
        }
    }
    else if (packet->type == DISPLAY_SET_TITLE)
    {
        string_copy(window->title, sizeof(window->title), packet->text);
    }
    else if (packet->type == DISPLAY_CLOSE_WINDOW)
    {
        if (window->surface)
        {
            shm_detach(window->shm_id);
        }
        window->used = false;
        if (state->focused == window_index(state, window))
        {
             state->focused = -1;
        }
    }
    else if (packet->type == DISPLAY_CLEAR_TEXT)
    {
        user_memset(window->lines, 0, sizeof(window->lines));
        window->line = 0;
        window->column = 0;
    }
}

/**
 * @brief Draws the mouse cursor on the display.
 * @param state Pointer to the display state structure.
 * @param dark The color for the dark part of the cursor.
 * @param light The color for the light part of the cursor.
 */
static void draw_cursor(const struct display_state* state, const uint32_t dark, const uint32_t light)
{
    for (int row = 0; row < 14; row++)
    {
        gfx_rect(&state->mode, state->backbuffer, state->mouse.x, state->mouse.y + row, 1 + row / 2, 1, light);
    }
    gfx_frame(&state->mode, state->backbuffer, state->mouse.x, state->mouse.y, 7, 13, dark);
}

/**
 * @brief Draws a window on the display.
 * @param state Pointer to the display state structure.
 * @param window Pointer to the display window structure.
 * @param focused Indicates whether the window is focused or not.
 */
static void draw_window(const struct display_state* state, const struct display_window* window, const bool focused)
{
    if (window->minimized) return;
    const uint32_t shadow = gfx_rgb(&state->mode, 17, 24, 39);
    const uint32_t frame = focused ? gfx_rgb(&state->mode, 56, 189, 248) : gfx_rgb(&state->mode, 71, 85, 105);
    const uint32_t title = focused ? gfx_rgb(&state->mode, 30, 64, 175) : gfx_rgb(&state->mode, 51, 65, 85);
    const uint32_t body = gfx_rgb(&state->mode, 8, 15, 28);
    const uint32_t white = gfx_rgb(&state->mode, 226, 232, 240);
    const uint32_t red = gfx_rgb(&state->mode, 248, 113, 113);
    const uint32_t green = gfx_rgb(&state->mode, 52, 211, 153);

    gfx_rect(&state->mode, state->backbuffer, window->x + 7, window->y + 7, window->width, window->height, shadow);
    gfx_rect(&state->mode, state->backbuffer, window->x, window->y, window->width, window->height, body);
    gfx_frame(&state->mode, state->backbuffer, window->x, window->y, window->width, window->height, frame);
    gfx_rect(&state->mode, state->backbuffer, window->x, window->y, window->width, 32, title);
    gfx_text(&state->mode, state->backbuffer, window->title, window->x + 12, window->y + 10, 1, white);
    gfx_rect(&state->mode, state->backbuffer, window->x + window->width - 27, window->y + 7, 18, 18, red);
    gfx_text(&state->mode, state->backbuffer, "X", window->x + window->width - 21, window->y + 12, 1, body);
    gfx_rect(&state->mode, state->backbuffer, window->x + window->width - 51, window->y + 7, 18, 18, green);
    gfx_text(&state->mode, state->backbuffer, window->maximized ? "-" : "+", window->x + window->width - 45, window->y + 12, 1, body);

    if (window->surface)
    {
        const uint32_t bytes = ((uint32_t)state->mode.bpp + 7U) / 8U;
        int copy_width = window->width - 2;
        int copy_height = window->height - 34;
        if (window->x + copy_width > (int)state->mode.width) copy_width = (int)state->mode.width - window->x;
        if (window->y + 33 + copy_height > (int)state->mode.height - PANEL_HEIGHT)
        {
            copy_height = (int)state->mode.height - PANEL_HEIGHT - window->y - 33;
        }
        for (int row = 0; row < copy_height; row++)
        {
            user_memcpy(state->backbuffer + (uint32_t)(window->y + 33 + row) * state->mode.pitch +
                        (uint32_t)(window->x + 1) * bytes,
                        window->surface + (uint32_t)row * (uint32_t)window->width * bytes,
                        (uint32_t)copy_width * bytes);
        }
        return;
    }

    const int available_lines = (window->height - 50) / 15;
    int first = window->line - available_lines + 1;
    if (first < 0) first = 0;
    int y = window->y + 43;
    for (int i = first; i <= window->line && i < DISPLAY_LINES; i++)
    {
        gfx_text(&state->mode, state->backbuffer, window->lines[i], window->x + 12, y, 1, white);
        y += 15;
    }
}

/**
 * @brief Draws the desktop interface on the display.
 * @param state Pointer to the display state structure.
 */
static void draw_desktop(const struct display_state* state)
{
    const uint32_t background = gfx_rgb(&state->mode, 13, 27, 42);
    const uint32_t background_light = gfx_rgb(&state->mode, 20, 45, 66);
    const uint32_t panel = gfx_rgb(&state->mode, 15, 23, 42);
    const uint32_t accent = gfx_rgb(&state->mode, 14, 165, 233);
    const uint32_t selected = gfx_rgb(&state->mode, 30, 64, 175);
    const uint32_t white = gfx_rgb(&state->mode, 241, 245, 249);
    const uint32_t muted = gfx_rgb(&state->mode, 148, 163, 184);
    const uint32_t card = gfx_rgb(&state->mode, 30, 52, 72);
    const uint32_t card_light = gfx_rgb(&state->mode, 38, 68, 91);
    const int screen_width = (int)state->mode.width;
    const int screen_height = (int)state->mode.height;

    gfx_rect(&state->mode, state->backbuffer, 0, 0, screen_width, screen_height, background);
    gfx_rect(&state->mode, state->backbuffer, 0, screen_height / 2, screen_width, screen_height / 2, background_light);

    gfx_rect(&state->mode, state->backbuffer, screen_width - 260, 92, 220, 12, selected);
    gfx_rect(&state->mode, state->backbuffer, screen_width - 220, 116, 180, 8, accent);
    gfx_rect(&state->mode, state->backbuffer, screen_width - 180, 136, 140, 5, muted);

    gfx_rect(&state->mode, state->backbuffer, 0, 0, screen_width, TOPBAR_HEIGHT, panel);
    gfx_rect(&state->mode, state->backbuffer, 0, TOPBAR_HEIGHT - 2, screen_width, 2, accent);
    gfx_rect(&state->mode, state->backbuffer, 14, 9, 24, 24, accent);
    gfx_text(&state->mode, state->backbuffer, "M", 23, 18, 1, panel);
    gfx_text(&state->mode, state->backbuffer, "MEXOS", 48, 13, 2, white);
    gfx_text(&state->mode, state->backbuffer, "DESKTOP SESSION", 160, 17, 1, muted);

    gfx_rect(&state->mode, state->backbuffer, screen_width - 132, 14, 8, 8, state->heartbeat ? gfx_rgb(&state->mode, 52, 211, 153) : muted);
    gfx_text(&state->mode, state->backbuffer, "LIVE", screen_width - 118, 17, 1, muted);

    char time[9];
    clock_text(state, time);
    gfx_text(&state->mode, state->backbuffer, time, screen_width > 100 ? screen_width - 78 : 4, 17, 1, white);

    gfx_text(&state->mode, state->backbuffer, "WORKSPACE", 32, 66, 1, muted);
    gfx_rect(&state->mode, state->backbuffer, LAUNCHER_X, LAUNCHER_Y, LAUNCHER_WIDTH, LAUNCHER_HEIGHT, card);
    gfx_frame(&state->mode, state->backbuffer, LAUNCHER_X, LAUNCHER_Y, LAUNCHER_WIDTH, LAUNCHER_HEIGHT, card_light);
    gfx_rect(&state->mode, state->backbuffer, LAUNCHER_X + 16, LAUNCHER_Y + 16, 54, 46, panel);
    gfx_frame(&state->mode, state->backbuffer, LAUNCHER_X + 16, LAUNCHER_Y + 16, 54, 46, accent);
    gfx_text(&state->mode, state->backbuffer, ">", LAUNCHER_X + 28, LAUNCHER_Y + 30, 2, accent);
    gfx_text(&state->mode, state->backbuffer, "TERMINAL", LAUNCHER_X + 82, LAUNCHER_Y + 28, 1, white);
    gfx_text(&state->mode, state->backbuffer, "OPEN SHELL", LAUNCHER_X + 82, LAUNCHER_Y + 49, 1, muted);
    gfx_text(&state->mode, state->backbuffer, "CLICK TO LAUNCH", LAUNCHER_X + 18, LAUNCHER_Y + 82, 1, accent);

    gfx_rect(&state->mode, state->backbuffer, 32, 218, 300, 100, card);
    gfx_text(&state->mode, state->backbuffer, "QUICK KEYS", 48, 236, 1, white);
    gfx_text(&state->mode, state->backbuffer, "T TERMINAL  F FILES  K TASKS", 48, 258, 1, muted);
    gfx_text(&state->mode, state->backbuffer, "C CALC  E SETTINGS  M MENU", 48, 275, 1, muted);
    gfx_text(&state->mode, state->backbuffer, state->pointer_available ? "POINTER PS2 READY" : "POINTER NEEDS USB OR I2C HID", 48, 298, 1, state->pointer_available ? accent : muted);

    for (uint32_t z = 1; z <= state->next_z; z++)
    {
        for (int i = 0; i < DISPLAY_MAX_WINDOWS; i++)
        {
            if (state->windows[i].used && state->windows[i].z == z)
            {
                draw_window(state, &state->windows[i], state->focused == i);
            }
        }
    }

    const int panel_y = screen_height - PANEL_HEIGHT;
    gfx_rect(&state->mode, state->backbuffer, 0, panel_y, (int)state->mode.width, PANEL_HEIGHT, panel);
    gfx_rect(&state->mode, state->backbuffer, 10, panel_y + 8, 78, 30, state->start_open ? selected : accent);
    gfx_text(&state->mode, state->backbuffer, "START", 24, panel_y + 19, 1, white);
    gfx_rect(&state->mode, state->backbuffer, 98, panel_y + 8, 92, 30, selected);
    gfx_text(&state->mode, state->backbuffer, "TERMINAL", 108, panel_y + 19, 1, white);
    gfx_rect(&state->mode, state->backbuffer, 198, panel_y + 8, 92, 30, selected);
    gfx_text(&state->mode, state->backbuffer, "CALC", 220, panel_y + 19, 1, white);

    int task_x = 294;
    for (int i = 0; i < DISPLAY_MAX_WINDOWS; i++)
    {
        if (!state->windows[i].used) continue;
        gfx_rect(&state->mode, state->backbuffer, task_x, panel_y + 8, 108, 30, state->focused == i && !state->windows[i].minimized ? selected : background);
        gfx_text(&state->mode, state->backbuffer, state->windows[i].title, task_x + 8, panel_y + 19, 1, white);
        task_x += 114;
    }

    if (state->start_open)
    {
        const int menu_y = panel_y - 220;
        gfx_rect(&state->mode, state->backbuffer, 10, menu_y, 220, 216, panel);
        gfx_frame(&state->mode, state->backbuffer, 10, menu_y, 220, 216, accent);
        gfx_text(&state->mode, state->backbuffer, "APPLICATIONS", 24, menu_y + 18, 1, muted);
        gfx_text(&state->mode, state->backbuffer, "TERMINAL", 24, menu_y + 50, 1, white);
        gfx_text(&state->mode, state->backbuffer, "FILES", 24, menu_y + 70, 1, white);
        gfx_text(&state->mode, state->backbuffer, "TASKS", 24, menu_y + 90, 1, white);
        gfx_text(&state->mode, state->backbuffer, "SETTINGS", 24, menu_y + 110, 1, white);
        gfx_text(&state->mode, state->backbuffer, state->confirm_action == DESKTOP_ACTION_REBOOT ? "CONFIRM REBOOT" : "REBOOT", 24, menu_y + 148, 1, white);
        gfx_text(&state->mode, state->backbuffer, state->confirm_action == DESKTOP_ACTION_SHUTDOWN ? "CONFIRM SHUTDOWN" : "SHUTDOWN", 24, menu_y + 168, 1, white);
    }

    struct system_info sys;
    user_memset(&sys, 0, sizeof(sys));
    sysinfo(&sys);

    const uint32_t red = gfx_rgb(&state->mode, 248, 113, 113);
    const uint32_t green = gfx_rgb(&state->mode, 52, 211, 153);
    const uint32_t yellow = gfx_rgb(&state->mode, 251, 191, 36);

    const int mem_percent = sys.total_memory_kb > 0 ? (sys.used_memory_kb * 100) / sys.total_memory_kb : 0;

    const uint32_t mem_color = mem_percent > 75 ? red : mem_percent > 50 ? yellow : green;

    char mem_text[32];
    user_memset(mem_text, 0, sizeof(mem_text));
    const char* fmt = "MEM:";
    int idx = 0;
    while (*fmt) mem_text[idx++] = *fmt++;
    mem_text[idx++] = ' ';
    if (mem_percent >= 10) mem_text[idx++] = (char)('0' + (mem_percent / 10));
    mem_text[idx++] = (char)('0' + (mem_percent % 10));
    mem_text[idx++] = '%';
    mem_text[idx] = '\0';

    gfx_text(&state->mode, state->backbuffer, mem_text, screen_width - 120, screen_height - 30, 1, mem_color);

    draw_cursor(state, background, white);
    if (state->backbuffer != state->framebuffer)
    {
        user_memcpy(state->framebuffer, state->backbuffer, state->mode.pitch * state->mode.height);
    }
}

/**
 * @brief Sends a desktop action event to the desktop port.
 * @param state Pointer to the display state structure.
 * @param action The action code to be sent.
 */
static void desktop_action(const struct display_state* state, const int action)
{
    struct display_packet packet;
    user_memset(&packet, 0, sizeof(packet));
    packet.type = DISPLAY_EVENT_ACTION;
    packet.a = action;
    send_packet(state->desktop_port, &packet);
}

/**
 * @brief Closes a window in the display state.
 * @param state Pointer to the display state structure.
 * @param index The index of the window to be closed.
 */
static void close_window(struct display_state* state, const int index)
{
    struct display_window* window = &state->windows[index];
    struct display_packet event;
    user_memset(&event, 0, sizeof(event));
    event.type = DISPLAY_EVENT_CLOSE;
    event.window_id = window->id;
    send_packet(window->event_port, &event);
    if (window->surface) shm_detach(window->shm_id);
    window->used = false;
    if (state->focused == index) state->focused = -1;
}

/**
 * @brief Handles a mouse click event on the display.
 * @param state Pointer to the display state structure.
 * @param x The x-coordinate of the mouse click.
 * @param y The y-coordinate of the mouse click.
 */
static void handle_click(struct display_state* state, const int x, const int y)
{
    const int panel_y = (int)state->mode.height - PANEL_HEIGHT;
    if (inside(x, y, 10, panel_y + 8, 78, 30))
    {
        state->start_open = !state->start_open;
        return;
    }
    if (inside(x, y, 198, panel_y + 8, 92, 30))
    {
        desktop_action(state, DESKTOP_ACTION_CALCULATOR);
        state->start_open = false;
        state->confirm_action = 0;
        return;
    }
    if (inside(x, y, 98, panel_y + 8, 92, 30))
    {
        desktop_action(state, DESKTOP_ACTION_TERMINAL);
        state->start_open = false;
        state->confirm_action = 0;
        return;
    }
    if (state->start_open)
    {
        const int menu_y = panel_y - 220;
        if (inside(x, y, 10, menu_y + 28, 220, 20))
        {
            desktop_action(state, DESKTOP_ACTION_TERMINAL);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (inside(x, y, 10, menu_y + 50, 220, 20))
        {
            desktop_action(state, DESKTOP_ACTION_FILE_MANAGER);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (inside(x, y, 10, menu_y + 70, 220, 20))
        {
            desktop_action(state, DESKTOP_ACTION_TASK_MANAGER);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (inside(x, y, 10, menu_y + 90, 220, 20))
        {
            desktop_action(state, DESKTOP_ACTION_SETTINGS);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (inside(x, y, 10, menu_y + 128, 220, 20))
        {
            if (state->confirm_action == DESKTOP_ACTION_REBOOT)
            {
                desktop_action(state, DESKTOP_ACTION_REBOOT);
            }
            else
            {
                state->confirm_action = DESKTOP_ACTION_REBOOT;
            }
        }
        else if (inside(x, y, 10, menu_y + 148, 220, 32))
        {
            if (state->confirm_action == DESKTOP_ACTION_SHUTDOWN)
            {
                desktop_action(state, DESKTOP_ACTION_SHUTDOWN);
            }
            else
            {
                state->confirm_action = DESKTOP_ACTION_SHUTDOWN;
            }
        }
        return;
    }

    int task_x = 202;
    for (int i = 0; i < DISPLAY_MAX_WINDOWS; i++)
    {
        if (!state->windows[i].used) continue;
        if (inside(x, y, task_x, panel_y + 8, 108, 30))
        {
            if (state->focused == i && !state->windows[i].minimized)
            {
                state->windows[i].minimized = true;
            }
            else
            {
                focus_window(state, i);
            }
            return;
        }
        task_x += 114;
    }

    int selected = -1;
    uint32_t best_z = 0;
    for (int i = 0; i < DISPLAY_MAX_WINDOWS; i++)
    {
        const struct display_window* window = &state->windows[i];
        if (window->used && !window->minimized &&
            inside(x, y, window->x, window->y, window->width, window->height) &&
            window->z >= best_z)
        {
            selected = i;
            best_z = window->z;
        }
    }
    if (selected < 0)
    {
        state->focused = -1;
        if (inside(x, y, LAUNCHER_X, LAUNCHER_Y, LAUNCHER_WIDTH, LAUNCHER_HEIGHT))
        {
            desktop_action(state, DESKTOP_ACTION_TERMINAL);
        }
        return;
    }
    struct display_window* window = &state->windows[selected];
    focus_window(state, selected);
    if (inside(x, y, window->x + window->width - 31, window->y, 31, 32))
    {
        close_window(state, selected);
        return;
    }
    if (inside(x, y, window->x + window->width - 58, window->y, 27, 32))
    {
        if (!window->maximized)
        {
            window->restore_x = window->x;
            window->restore_y = window->y;
            window->restore_width = window->width;
            window->restore_height = window->height;
            window->x = 4;
            window->y = TOPBAR_HEIGHT + 4;
            window->width = (int)state->mode.width - 8;
            window->height = (int)state->mode.height - PANEL_HEIGHT - TOPBAR_HEIGHT - 8;
            window->maximized = true;
        }
        else
        {
            window->x = window->restore_x;
            window->y = window->restore_y;
            window->width = window->restore_width;
            window->height = window->restore_height;
            window->maximized = false;
        }
        return;
    }
    if (inside(x, y, window->x, window->y + 33, window->width, window->height -32))
    {
        struct display_packet click;
        user_memset(&click, 0, sizeof(click));
        click.type = DISPLAY_EVENT_CLICK;
        click.window_id = window->id;
        click.a = x - window->x;
        click.b = y - (window->y + 33);
        send_packet(window->event_port, &click);
        return;
    }
    if (inside(x, y, window->x + window->width - 14, window->y + window->height - 14, 14, 14))
    {
        state->resizing = true;
        state->drag_window = selected;
        return;
    }
    if (inside(x, y, window->x, window->y, window->width - 58, 32))
    {
        state->dragging = true;
        state->drag_window = selected;
        state->drag_dx = x - window->x;
        state->drag_dy = y - window->y;
    }
}

/**
 * @brief Routes a key event to the appropriate window or handles desktop actions.
 * @param state Pointer to the display state structure.
 * @param key The key code of the pressed key.
 */
static void route_key(struct display_state* state, const unsigned char key)
{
    if (state->focused < 0 || !state->windows[state->focused].used)
    {
        if (key == 't' || key == 'T')
        {
            desktop_action(state, DESKTOP_ACTION_TERMINAL);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (key == 'm' || key == 'M')
        {
            state->start_open = !state->start_open;
            state->confirm_action = 0;
        }
        else if (key == 'c' || key == 'C')
        {
            desktop_action(state, DESKTOP_ACTION_CALCULATOR);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (key == 'f' || key == 'F')
        {
            desktop_action(state, DESKTOP_ACTION_FILE_MANAGER);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (key == 'k' || key == 'K')
        {
            desktop_action(state, DESKTOP_ACTION_TASK_MANAGER);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (key == 'e' || key == 'E')
        {
            desktop_action(state, DESKTOP_ACTION_SETTINGS);
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (key == 27)
        {
            state->start_open = false;
            state->confirm_action = 0;
        }
        else if (state->start_open && (key == 'r' || key == 'R'))
        {
            if (state->confirm_action == DESKTOP_ACTION_REBOOT)
            {
                desktop_action(state, DESKTOP_ACTION_REBOOT);
            }
            else
            {
                state->confirm_action = DESKTOP_ACTION_REBOOT;
            }
        }
        else if (state->start_open && (key == 's' || key == 'S'))
        {
            if (state->confirm_action == DESKTOP_ACTION_SHUTDOWN)
            {
                desktop_action(state, DESKTOP_ACTION_SHUTDOWN);
            }
            else
            {
                state->confirm_action = DESKTOP_ACTION_SHUTDOWN;
            }
        }
        return;
    }
    struct display_packet event;
    user_memset(&event, 0, sizeof(event));
    event.type = DISPLAY_EVENT_KEY;
    event.window_id = state->windows[state->focused].id;
    event.a = key;
    send_packet(state->windows[state->focused].event_port, &event);
}

/**
 * @brief Gets the current number of ticks.
 * @return The current number of ticks.
 */
static inline uint64_t get_ticks(void)
{
    uint32_t low;
    uint32_t high;

    ASM_V("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

/**
 * @brief The main entry point of the display daemon.
 * @return Exit status code.
 */
int main(void)
{
    const int server_port = display_claim();
    if (server_port != DISPLAY_SERVER_PORT)
    {
        user_println("displayd: display service unavailable");
        return 1;
    }

    static struct display_state state;
    user_memset(&state, 0, sizeof(state));
    state.desktop_port = -1;
    state.focused = -1;
    state.drag_window = -1;
    state.framebuffer = mmap_fb(&state.mode);
    if (!state.framebuffer ||
        (state.mode.bpp != 15 && state.mode.bpp != 16 &&
         state.mode.bpp != 24 && state.mode.bpp != 32))
    {
        user_println("displayd: framebuffer unavailable");
        return 1;
    }
    state.backbuffer = mmap_anon(state.mode.pitch * state.mode.height);
    if (!state.backbuffer) state.backbuffer = state.framebuffer;

    struct mouse_state initial_mouse;
    user_memset(&initial_mouse, 0, sizeof(initial_mouse));
    if (poll_mouse(&initial_mouse) > 0)
    {
        state.mouse = initial_mouse;
        state.pointer_available = true;
    }
    state.clock_valid = gettime(&state.clock) == 0;

    *(uint64_t*)&state.frame_interval_ticks = state.mode.width > 0 ? 2400000000UL / 60 : 0;
    state.last_frame_ticks = get_ticks();
    state.mouse_position_only = false;

    draw_desktop(&state);

    bool dirty = false;
    bool boot_diagnostic = false;
    while (1)
    {
        if (boot_diagnostic) draw_boot_stage(&state, "LOOP");

        const bool heartbeat = compositor_heartbeat();
        if (heartbeat != state.heartbeat)
        {
            state.heartbeat = heartbeat;
            dirty = true;
        }

        if (boot_diagnostic) draw_boot_stage(&state, "IPC");
        struct message message;
        while (recv(server_port, &message, IPC_NONBLOCK) == 0)
        {
            if (message.len >= sizeof(struct display_packet))
            {
                handle_packet(&state, (const struct display_packet*)message.data);
            }
            dirty = true;
            state.mouse_position_only = false;
        }

        if (boot_diagnostic) draw_boot_stage(&state, "MOUSE");
        struct mouse_state mouse;
        const int mouse_result = poll_mouse(&mouse);
        if (mouse_result > 0)
        {
            if (!state.pointer_available)
            {
                state.pointer_available = true;
                dirty = true;
                state.mouse_position_only = false;
            }
            if (mouse.x != state.mouse.x || mouse.y != state.mouse.y ||
                mouse.buttons != state.mouse.buttons)
            {
                state.mouse = mouse;
                const bool down = (mouse.buttons & MOUSE_LEFT_BUTTON) != 0;
                if (down && !state.mouse_down)
                {
                    handle_click(&state, mouse.x, mouse.y);
                    state.mouse_position_only = false;
                }
                if (!down)
                {
                    state.dragging = false;
                    state.resizing = false;
                }
                if (down && state.dragging && state.drag_window >= 0)
                {
                    struct display_window* window = &state.windows[state.drag_window];
                    window->x = mouse.x - state.drag_dx;
                    window->y = mouse.y - state.drag_dy;
                    if (window->x < 0) window->x = 0;
                    if (window->y < 0) window->y = 0;
                    state.mouse_position_only = true;
                    dirty = true;
                }
                if (down && state.resizing && state.drag_window >= 0)
                {
                    struct display_window* window = &state.windows[state.drag_window];
                    window->width = mouse.x - window->x;
                    window->height = mouse.y - window->y;
                    if (window->width < 260) window->width = 260;
                    if (window->height < 160) window->height = 160;
                    state.mouse_position_only = true;
                    dirty = true;
                }
                state.mouse_down = down;
                if (!state.mouse_position_only)
                {
                    dirty = true;
                }
            }
        }

        if (boot_diagnostic) draw_boot_stage(&state, "KEY");
        unsigned char key = 0;

        for (int input_count = 0; input_count < 64 && poll_key(&key) > 0; input_count++)
        {
            route_key(&state, key);
            dirty = true;
            state.mouse_position_only = false;
        }

        if (boot_diagnostic) draw_boot_stage(&state, "RTC");
        struct rtc_time now;
        if (gettime(&now) == 0 &&
            (!state.clock_valid ||
             now.second != state.clock.second ||
             now.minute != state.clock.minute ||
             now.hour != state.clock.hour))
        {
            state.clock = now;
            state.clock_valid = true;
            dirty = true;
            state.mouse_position_only = false;
        }

        if (boot_diagnostic) draw_boot_stage(&state, "DRAW");
        uint64_t now_ticks = get_ticks();
        if (now_ticks - state.last_frame_ticks >= state.frame_interval_ticks)
        {
            if (dirty)
            {
                draw_desktop(&state);
                dirty = false;
            }
            state.last_frame_ticks = now_ticks;
        }
        else if (state.mouse_position_only && dirty)
        {
            draw_desktop(&state);
            dirty = false;
        }

        if (boot_diagnostic) draw_boot_stage(&state, "YIELD");
        yield();
        if (boot_diagnostic)
        {
            draw_boot_stage(&state, "RUN");
            boot_diagnostic = false;
        }
    }
}
