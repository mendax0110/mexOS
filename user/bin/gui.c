#include "runtime.h"

#define GUI_MAX_FILES 32
#define GUI_LOG_LINES 9
#define GUI_LOG_LEN 72
#define GUI_CMD_LEN 96
#define GUI_PATH_LEN 128

#define GUI_PWR_BTN_W 34
#define GUI_PWR_BTN_H 28
#define GUI_PWR_GAP 6
#define GUI_PWR_MARGIN 10
#define GUI_PWR_Y 10

#define KEY_ESC 27
#define KEY_TAB 9
#define KEY_ENTER '\n'
#define KEY_BACKSPACE '\b'
#define KEY_ARROW_UP 0x80
#define KEY_ARROW_DOWN 0x81
#define KEY_ARROW_LEFT 0x82
#define KEY_ARROW_RIGHT 0x83

/**
 * @brief Enum to represent the different views of the GUI \enum gui_view
 */
enum gui_view
{
    GUI_VIEW_HOME = 0,
    GUI_VIEW_FILES = 1,
    GUI_VIEW_TERMINAL = 2,
    GUI_VIEW_SYSTEM = 3,
    GUI_VIEW_COUNT = 4
};

/**
 * @brief Struct to represent the color palette of the GUI \struct gui_palette
 */
struct gui_palette
{
    uint32_t bg;
    uint32_t bg2;
    uint32_t top;
    uint32_t dock;
    uint32_t panel;
    uint32_t panel2;
    uint32_t edge;
    uint32_t ink;
    uint32_t muted;
    uint32_t white;
    uint32_t teal;
    uint32_t coral;
    uint32_t gold;
    uint32_t green;
    uint32_t shadow;
};

/**
 * @brief Struct to represent the state of the user-land user interface \struct gui_state
 */
struct gui_state
{
    enum gui_view view;
    int launcher_index;
    int file_index;
    char cwd[GUI_PATH_LEN];
    struct fs_dirent files[GUI_MAX_FILES];
    int file_count;
    char command[GUI_CMD_LEN];
    size_t command_len;
    char log[GUI_LOG_LINES][GUI_LOG_LEN];
    char status[GUI_LOG_LEN];
    unsigned char escape_state;
    bool running;
    struct mouse_state mouse;
    bool mouse_was_down;
};

static uint32_t min_u32(const uint32_t a, const uint32_t b)
{
    return a < b ? a : b;
}

static uint32_t channel(const uint8_t value, const uint8_t size, const uint8_t pos)
{
    if (size == 0)
    {
        return 0;
    }

    uint32_t scaled = value;
    if (size < 8)
    {
        scaled >>= (uint8_t)(8 - size);
    }
    else if (size > 8)
    {
        scaled <<= (uint8_t)(size - 8);
    }

    return scaled << pos;
}

static uint32_t rgb(const struct vesa_mode_info* info, const uint8_t r, const uint8_t g, const uint8_t b)
{
    return channel(r, info->red_size, info->red_pos) |
           channel(g, info->green_size, info->green_pos) |
           channel(b, info->blue_size, info->blue_pos);
}

static void make_palette(const struct vesa_mode_info* info, struct gui_palette* p)
{
    p->bg = rgb(info, 28, 39, 54);
    p->bg2 = rgb(info, 43, 65, 78);
    p->top = rgb(info, 11, 15, 24);
    p->dock = rgb(info, 20, 31, 45);
    p->panel = rgb(info, 235, 239, 242);
    p->panel2 = rgb(info, 248, 250, 252);
    p->edge = rgb(info, 91, 105, 117);
    p->ink = rgb(info, 22, 29, 38);
    p->muted = rgb(info, 92, 105, 119);
    p->white = rgb(info, 255, 255, 255);
    p->teal = rgb(info, 38, 181, 174);
    p->coral = rgb(info, 231, 94, 86);
    p->gold = rgb(info, 239, 180, 72);
    p->green = rgb(info, 90, 177, 102);
    p->shadow = rgb(info, 7, 10, 15);
}

static void put_pixel(const struct vesa_mode_info* info, uint8_t* fb,
                      const uint32_t x, const uint32_t y, const uint32_t color)
{
    if (!info || !fb || x >= info->width || y >= info->height)
    {
        return;
    }

    const uint32_t bytes_per_pixel = info->bpp / 8U;
    uint8_t* pixel = fb + (y * info->pitch) + (x * bytes_per_pixel);

    if (info->bpp == 32)
    {
        *((uint32_t*)pixel) = color;
    }
    else if (info->bpp == 24)
    {
        pixel[0] = (uint8_t)((color >> 0U) & 0xFFU);
        pixel[1] = (uint8_t)((color >> 8U) & 0xFFU);
        pixel[2] = (uint8_t)((color >> 16U) & 0xFFU);
    }
}

static void fill_rect(const struct vesa_mode_info* info, uint8_t* fb,
                      const uint32_t x, const uint32_t y, uint32_t width, uint32_t height,
                      const uint32_t color)
{
    if (!info || !fb || x >= info->width || y >= info->height)
    {
        return;
    }

    width = min_u32(width, info->width - x);
    height = min_u32(height, info->height - y);

    if (info->bpp == 32)
    {
        for (uint32_t row = 0; row < height; row++)
        {
            uint32_t* pixels = (uint32_t*)(fb + ((y + row) * info->pitch) + (x * 4U));
            for (uint32_t col = 0; col < width; col++)
            {
                pixels[col] = color;
            }
        }
        return;
    }

    if (info->bpp == 24)
    {
        const uint8_t b = (uint8_t)((color >> 0U) & 0xFFU);
        const uint8_t g = (uint8_t)((color >> 8U) & 0xFFU);
        const uint8_t r = (uint8_t)((color >> 16U) & 0xFFU);
        for (uint32_t row = 0; row < height; row++)
        {
            uint8_t* pixel = fb + ((y + row) * info->pitch) + (x * 3U);
            for (uint32_t col = 0; col < width; col++)
            {
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
                pixel += 3;
            }
        }
        return;
    }

    for (uint32_t row = 0; row < height; row++)
    {
        for (uint32_t col = 0; col < width; col++)
        {
            put_pixel(info, fb, x + col, y + row, color);
        }
    }
}

static void stroke_rect(const struct vesa_mode_info* info, uint8_t* fb,
                        const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height,
                        const uint32_t color)
{
    if (width < 2 || height < 2)
    {
        return;
    }

    fill_rect(info, fb, x, y, width, 1, color);
    fill_rect(info, fb, x, y + height - 1U, width, 1, color);
    fill_rect(info, fb, x, y, 1, height, color);
    fill_rect(info, fb, x + width - 1U, y, 1, height, color);
}

static uint8_t glyph_row(char c, const uint32_t row)
{
    if (c >= 'a' && c <= 'z')
    {
        c = (char)(c - ('a' - 'A'));
    }

    if (row >= 7)
    {
        return 0;
    }

    switch (c)
    {
        case 'A': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g[row]; }
        case 'B': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E }; return g[row]; }
        case 'C': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F }; return g[row]; }
        case 'D': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E }; return g[row]; }
        case 'E': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }; return g[row]; }
        case 'F': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 }; return g[row]; }
        case 'G': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0F }; return g[row]; }
        case 'H': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g[row]; }
        case 'I': { static const uint8_t g[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F }; return g[row]; }
        case 'J': { static const uint8_t g[7] = { 0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C }; return g[row]; }
        case 'K': { static const uint8_t g[7] = { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }; return g[row]; }
        case 'L': { static const uint8_t g[7] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }; return g[row]; }
        case 'M': { static const uint8_t g[7] = { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }; return g[row]; }
        case 'N': { static const uint8_t g[7] = { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }; return g[row]; }
        case 'O': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g[row]; }
        case 'P': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }; return g[row]; }
        case 'Q': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D }; return g[row]; }
        case 'R': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }; return g[row]; }
        case 'S': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E }; return g[row]; }
        case 'T': { static const uint8_t g[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }; return g[row]; }
        case 'U': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g[row]; }
        case 'V': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 }; return g[row]; }
        case 'W': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A }; return g[row]; }
        case 'X': { static const uint8_t g[7] = { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 }; return g[row]; }
        case 'Y': { static const uint8_t g[7] = { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 }; return g[row]; }
        case 'Z': { static const uint8_t g[7] = { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F }; return g[row]; }
        case '0': { static const uint8_t g[7] = { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E }; return g[row]; }
        case '1': { static const uint8_t g[7] = { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }; return g[row]; }
        case '2': { static const uint8_t g[7] = { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F }; return g[row]; }
        case '3': { static const uint8_t g[7] = { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E }; return g[row]; }
        case '4': { static const uint8_t g[7] = { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }; return g[row]; }
        case '5': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E }; return g[row]; }
        case '6': { static const uint8_t g[7] = { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E }; return g[row]; }
        case '7': { static const uint8_t g[7] = { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }; return g[row]; }
        case '8': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E }; return g[row]; }
        case '9': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }; return g[row]; }
        case '.': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C }; return g[row]; }
        case ':': { static const uint8_t g[7] = { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 }; return g[row]; }
        case '-': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }; return g[row]; }
        case '_': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F }; return g[row]; }
        case '/': { static const uint8_t g[7] = { 0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10 }; return g[row]; }
        case '>': { static const uint8_t g[7] = { 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10 }; return g[row]; }
        case '<': { static const uint8_t g[7] = { 0x01, 0x02, 0x04, 0x08, 0x04, 0x02, 0x01 }; return g[row]; }
        case '[': { static const uint8_t g[7] = { 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E }; return g[row]; }
        case ']': { static const uint8_t g[7] = { 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E }; return g[row]; }
        case '+': { static const uint8_t g[7] = { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 }; return g[row]; }
        case '*': { static const uint8_t g[7] = { 0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00 }; return g[row]; }
        case '$': { static const uint8_t g[7] = { 0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04 }; return g[row]; }
        case ' ': return 0;
        default: { static const uint8_t g[7] = { 0x1F, 0x11, 0x05, 0x02, 0x04, 0x00, 0x04 }; return g[row]; }
    }
}

static void draw_char(const struct vesa_mode_info* info, uint8_t* fb,
                      const char c, const uint32_t x, const uint32_t y,
                      const uint32_t scale, const uint32_t color)
{
    for (uint32_t row = 0; row < 7; row++)
    {
        const uint8_t bits = glyph_row(c, row);
        for (uint32_t col = 0; col < 5; col++)
        {
            if (bits & (uint8_t)(1U << (4U - col)))
            {
                fill_rect(info, fb, x + (col * scale), y + (row * scale), scale, scale, color);
            }
        }
    }
}

static void draw_text(const struct vesa_mode_info* info, uint8_t* fb,
                      const char* text, uint32_t x, const uint32_t y,
                      const uint32_t scale, const uint32_t color)
{
    while (*text)
    {
        draw_char(info, fb, *text, x, y, scale, color);
        x += 6U * scale;
        text++;
    }
}

static void copy_string(char* dst, const size_t dst_size, const char* src)
{
    if (!dst || dst_size == 0)
    {
        return;
    }

    size_t i = 0;
    if (src)
    {
        while (src[i] && i + 1 < dst_size)
        {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static void append_string(char* dst, const size_t dst_size, const char* src)
{
    if (!dst || !src || dst_size == 0)
    {
        return;
    }

    size_t len = user_strlen(dst);
    while (*src && len + 1 < dst_size)
    {
        dst[len++] = *src++;
    }
    dst[len] = '\0';
}

static bool starts_with(const char* text, const char* prefix)
{
    while (*prefix)
    {
        if (*text++ != *prefix++)
        {
            return false;
        }
    }
    return true;
}

static void int_to_dec(int value, char* out, const size_t out_size)
{
    if (!out || out_size == 0)
    {
        return;
    }

    if (value == 0)
    {
        copy_string(out, out_size, "0");
        return;
    }

    char temp[12];
    int pos = 0;
    bool neg = false;
    if (value < 0)
    {
        neg = true;
        value = -value;
    }

    while (value > 0 && pos < (int)sizeof(temp))
    {
        temp[pos++] = (char)('0' + (value % 10));
        value /= 10;
    }

    size_t out_pos = 0;
    if (neg && out_pos + 1 < out_size)
    {
        out[out_pos++] = '-';
    }

    while (pos > 0 && out_pos + 1 < out_size)
    {
        out[out_pos++] = temp[--pos];
    }
    out[out_pos] = '\0';
}

static void log_line(struct gui_state* state, const char* text)
{
    for (int i = 0; i < GUI_LOG_LINES - 1; i++)
    {
        copy_string(state->log[i], sizeof(state->log[i]), state->log[i + 1]);
    }
    copy_string(state->log[GUI_LOG_LINES - 1], sizeof(state->log[0]), text);
}

static void set_status(struct gui_state* state, const char* text)
{
    copy_string(state->status, sizeof(state->status), text);
}

static void set_key_status(struct gui_state* state, const unsigned char key)
{
    char value[16];
    int_to_dec((int)key, value, sizeof(value));
    copy_string(state->status, sizeof(state->status), "KEY ");
    append_string(state->status, sizeof(state->status), value);
}

static void update_cwd(struct gui_state* state)
{
    if (getcwd(state->cwd, sizeof(state->cwd)) < 0)
    {
        copy_string(state->cwd, sizeof(state->cwd), "/");
    }
}

static void refresh_files(struct gui_state* state)
{
    update_cwd(state);
    const int count = readdir(state->cwd, state->files, GUI_MAX_FILES);
    state->file_count = count < 0 ? 0 : count;
    if (state->file_index >= state->file_count)
    {
        state->file_index = state->file_count > 0 ? state->file_count - 1 : 0;
    }
}

static void pad2(char* out, const size_t out_size, const uint32_t value)
{
    if (out_size < 3)
    {
        return;
    }

    out[0] = (char)('0' + ((value / 10) % 10));
    out[1] = (char)('0' + (value % 10));
    out[2] = '\0';
}

static void time_label(char* out, const size_t out_size)
{
    struct rtc_time now;
    if (gettime(&now) != 0)
    {
        copy_string(out, out_size, "READY");
        return;
    }

    char h[3], m[3], s[3];
    pad2(h, sizeof(h), now.hour);
    pad2(m, sizeof(m), now.minute);
    pad2(s, sizeof(s), now.second);

    copy_string(out, out_size, h);
    append_string(out, out_size, ":");
    append_string(out, out_size, m);
    append_string(out, out_size, ":");
    append_string(out, out_size, s);
}

static void draw_background(const struct vesa_mode_info* info, uint8_t* fb, const struct gui_palette* p)
{
    const uint32_t top_h = info->height / 2U;
    fill_rect(info, fb, 0, 0, info->width, top_h, p->bg);
    fill_rect(info, fb, 0, top_h, info->width, info->height - top_h, p->bg2);
}

static void draw_power_button(const struct vesa_mode_info* info, uint8_t* fb,
                              const struct gui_palette* p, const uint32_t x,
                              const uint32_t color, const char* label)
{
    fill_rect(info, fb, x, GUI_PWR_Y, GUI_PWR_BTN_W, GUI_PWR_BTN_H, p->dock);
    stroke_rect(info, fb, x, GUI_PWR_Y, GUI_PWR_BTN_W, GUI_PWR_BTN_H, color);
    fill_rect(info, fb, x + 11, GUI_PWR_Y + 7, 12, 12, color);
    draw_text(info, fb, label, x + 14, GUI_PWR_Y + 9, 1, p->white);
}

static uint32_t power_reboot_x(const struct vesa_mode_info* info)
{
    return info->width > (GUI_PWR_MARGIN + GUI_PWR_BTN_W)
            ? info->width - GUI_PWR_MARGIN - GUI_PWR_BTN_W
            : 0;
}

static uint32_t power_shutdown_x(const struct vesa_mode_info* info)
{
    const uint32_t rx = power_reboot_x(info);
    return rx > (GUI_PWR_BTN_W + GUI_PWR_GAP) ? rx - GUI_PWR_BTN_W - GUI_PWR_GAP : 0;
}

static void draw_power_options(const struct vesa_mode_info* info, uint8_t* fb,
                                   const struct gui_palette* p, const struct gui_state* state)
{
    UNUSED(state, "power buttons have no selection state yet");
    draw_power_button(info, fb, p, power_shutdown_x(info), p->coral, "P");
    draw_power_button(info, fb, p, power_reboot_x(info), p->gold, "R");
}

static void draw_topbar(const struct vesa_mode_info* info, uint8_t* fb,
                        const struct gui_palette* p, const struct gui_state* state)
{
    fill_rect(info, fb, 0, 0, info->width, 48, p->top);
    fill_rect(info, fb, 0, 46, info->width, 2, p->teal);
    draw_text(info, fb, "MEXOS", 18, 14, 2, p->white);

    const char* view_name = "HOME";
    if (state->view == GUI_VIEW_FILES) view_name = "FILES";
    if (state->view == GUI_VIEW_TERMINAL) view_name = "TERMINAL";
    if (state->view == GUI_VIEW_SYSTEM) view_name = "SYSTEM";

    draw_text(info, fb, view_name, 136, 18, 1, p->gold);
    draw_text(info, fb, state->status, 230, 18, 1, p->white);

    char label[10];
    time_label(label, sizeof(label));
    const uint32_t label_w = 6U * user_strlen(label);
    const uint32_t pwr_left = power_shutdown_x(info);
    const uint32_t label_gap = 12;
    const uint32_t x = pwr_left > (label_w + label_gap) ? pwr_left - label_w - label_gap : 0;
    draw_text(info, fb, label, x, 18, 1, p->white);

    draw_power_options(info, fb, p, state);
}

static void draw_dock_icon(const struct vesa_mode_info* info, uint8_t* fb,
                           const struct gui_palette* p, const uint32_t y,
                           const uint32_t color, const char* label, const bool active)
{
    fill_rect(info, fb, 14, y, 50, 42, active ? p->panel2 : p->dock);
    stroke_rect(info, fb, 14, y, 50, 42, active ? p->teal : p->edge);
    fill_rect(info, fb, 25, y + 8, 28, 20, color);
    draw_text(info, fb, label, 18, y + 31, 1, active ? p->ink : p->white);
}

static void draw_dock(const struct vesa_mode_info* info, uint8_t* fb,
                      const struct gui_palette* p, const struct gui_state* state)
{
    fill_rect(info, fb, 0, 48, 80, info->height > 48 ? info->height - 48 : 0, p->dock);
    draw_dock_icon(info, fb, p, 72, p->teal, "HOME", state->view == GUI_VIEW_HOME);
    draw_dock_icon(info, fb, p, 128, p->gold, "FILE", state->view == GUI_VIEW_FILES);
    draw_dock_icon(info, fb, p, 184, p->green, "TERM", state->view == GUI_VIEW_TERMINAL);
    draw_dock_icon(info, fb, p, 240, p->coral, "SYS", state->view == GUI_VIEW_SYSTEM);
}

static void draw_window(const struct vesa_mode_info* info, uint8_t* fb, const struct gui_palette* p,
                        const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h,
                        const char* title)
{
    fill_rect(info, fb, x + 7, y + 7, w, h, p->shadow);
    fill_rect(info, fb, x, y, w, h, p->panel);
    stroke_rect(info, fb, x, y, w, h, p->edge);
    fill_rect(info, fb, x, y, w, 34, rgb(info, 35, 45, 58));
    fill_rect(info, fb, x + 12, y + 10, 12, 12, p->coral);
    fill_rect(info, fb, x + 32, y + 10, 12, 12, p->gold);
    fill_rect(info, fb, x + 52, y + 10, 12, 12, p->teal);
    draw_text(info, fb, title, x + 78, y + 12, 1, p->white);
}

static void draw_button(const struct vesa_mode_info* info, uint8_t* fb, const struct gui_palette* p,
                        const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h,
                        const char* title, const char* subtitle, const uint32_t accent, const bool selected)
{
    fill_rect(info, fb, x, y, w, h, selected ? rgb(info, 255, 255, 255) : p->panel2);
    stroke_rect(info, fb, x, y, w, h, selected ? p->teal : p->edge);
    fill_rect(info, fb, x, y, 8, h, accent);
    draw_text(info, fb, title, x + 22, y + 18, 2, p->ink);
    draw_text(info, fb, subtitle, x + 24, y + 58, 1, p->muted);
}

static void draw_home(const struct vesa_mode_info* info, uint8_t* fb,
                      const struct gui_palette* p, const struct gui_state* state)
{
    const uint32_t x = 110;
    const uint32_t y = 82;
    const uint32_t w = info->width > 150 ? info->width - 150 : info->width - 20;
    const uint32_t h = info->height > 120 ? info->height - 132 : info->height / 2U;
    draw_window(info, fb, p, x, y, w, h, "DESKTOP");

    draw_text(info, fb, "USER SPACE GUI SESSION", x + 32, y + 60, 2, p->ink);
    draw_text(info, fb, "TAB OR 1-4 SWITCHES APPS. ENTER OPENS SELECTED.", x + 34, y + 104, 1, p->muted);

    const uint32_t card_w = w > 72 ? (w - 72) / 2U : w;
    draw_button(info, fb, p, x + 32, y + 140, card_w, 86, "FILES", "BROWSE VFS", p->gold, state->launcher_index == 0);
    draw_button(info, fb, p, x + 52 + card_w, y + 140, card_w, 86, "TERM", "GUI COMMANDS", p->green, state->launcher_index == 1);
    draw_button(info, fb, p, x + 32, y + 246, card_w, 86, "SYSTEM", "POWER AND SHELL", p->coral, state->launcher_index == 2);
    draw_button(info, fb, p, x + 52 + card_w, y + 246, card_w, 86, "SHELL", "EXEC /BIN/SH", p->teal, state->launcher_index == 3);
}

static void draw_files(const struct vesa_mode_info* info, uint8_t* fb,
                       const struct gui_palette* p, const struct gui_state* state)
{
    const uint32_t x = 110;
    const uint32_t y = 82;
    const uint32_t w = info->width > 150 ? info->width - 150 : info->width - 20;
    const uint32_t h = info->height > 120 ? info->height - 132 : info->height / 2U;
    draw_window(info, fb, p, x, y, w, h, "FILES");
    draw_text(info, fb, "PATH", x + 28, y + 58, 1, p->muted);
    draw_text(info, fb, state->cwd, x + 86, y + 58, 1, p->ink);

    const uint32_t list_top = y + 92;
    const uint32_t footer_h = 32;
    const uint32_t list_bottom = (y + h > footer_h) ? (y + h - footer_h) : list_top;
    const uint32_t row_h = 28;

    uint32_t visible_rows = (list_bottom > list_top) ? (list_bottom - list_top) / row_h : 0;
    if (visible_rows == 0)
    {
        visible_rows = 1;
    }

    int scroll_offset = 0;
    if (state->file_count > (int)visible_rows)
    {
        if (state->file_index >= (int)visible_rows)
        {
            scroll_offset = state->file_index - (int)visible_rows + 1;
        }

        const int max_offset = state->file_count - (int)visible_rows;
        if (scroll_offset > max_offset)
        {
            scroll_offset = max_offset;
        }
        if (scroll_offset < 0)
        {
            scroll_offset = 0;
        }
    }

    uint32_t row_y = list_top;
    const int last = min_u32((uint32_t)(scroll_offset + (int)visible_rows), (uint32_t)state->file_count);
    for (int i = scroll_offset; i < last && i < GUI_MAX_FILES; i++)
    {
        const bool selected = i == state->file_index;
        fill_rect(info, fb, x + 24, row_y, w > 48 ? w - 48 : w, 24, selected ? rgb(info, 210, 244, 241) : p->panel);
        const uint32_t icon = state->files[i].type == FS_ABI_TYPE_DIR ? p->gold : p->teal;
        fill_rect(info, fb, x + 32, row_y + 6, 12, 12, icon);
        draw_text(info, fb, state->files[i].type == FS_ABI_TYPE_DIR ? "[DIR]" : "[BIN]", x + 54, row_y + 7, 1, p->muted);
        draw_text(info, fb, state->files[i].name, x + 116, row_y + 7, 1, p->ink);
        row_y += row_h;
    }

    if (state->file_count == 0)
    {
        draw_text(info, fb, "NO ENTRIES", x + 32, list_top, 1, p->muted);
    }
    else if (state->file_count > (int)visible_rows)
    {
        char scroll_label[24];
        char num[8];
        copy_string(scroll_label, sizeof(scroll_label), "");
        int_to_dec(state->file_index + 1, num, sizeof(num));
        append_string(scroll_label, sizeof(scroll_label), num);
        append_string(scroll_label, sizeof(scroll_label), "/");
        int_to_dec(state->file_count, num, sizeof(num));
        append_string(scroll_label, sizeof(scroll_label), num);
        draw_text(info, fb, scroll_label, x + w - 68, y + 58, 1, p->muted);
    }

    draw_text(info, fb, "UP DOWN SELECT. ENTER OPENS. BACKSPACE GOES UP.", x + 28, y + h - 24, 1, p->muted);
}

static void draw_terminal(const struct vesa_mode_info* info, uint8_t* fb,
                          const struct gui_palette* p, const struct gui_state* state)
{
    const uint32_t x = 110;
    const uint32_t y = 82;
    const uint32_t w = info->width > 150 ? info->width - 150 : info->width - 20;
    const uint32_t h = info->height > 120 ? info->height - 132 : info->height / 2U;
    draw_window(info, fb, p, x, y, w, h, "TERMINAL");

    fill_rect(info, fb, x + 24, y + 54, w > 48 ? w - 48 : w, h > 86 ? h - 86 : h / 2U, rgb(info, 12, 18, 25));
    stroke_rect(info, fb, x + 24, y + 54, w > 48 ? w - 48 : w, h > 86 ? h - 86 : h / 2U, p->edge);

    uint32_t line_y = y + 70;
    for (int i = 0; i < GUI_LOG_LINES; i++)
    {
        draw_text(info, fb, state->log[i], x + 38, line_y, 1, p->white);
        line_y += 18;
    }

    fill_rect(info, fb, x + 36, y + h - 58, w > 72 ? w - 72 : w, 28, p->panel2);
    stroke_rect(info, fb, x + 36, y + h - 58, w > 72 ? w - 72 : w, 28, p->teal);
    draw_text(info, fb, ">", x + 48, y + h - 50, 1, p->ink);
    draw_text(info, fb, state->command, x + 66, y + h - 50, 1, p->ink);
    fill_rect(info, fb, x + 70 + (uint32_t)state->command_len * 6U, y + h - 50, 2, 10, p->coral);
}

static void draw_system(const struct vesa_mode_info* info, uint8_t* fb,
                        const struct gui_palette* p)
{
    const uint32_t x = 110;
    const uint32_t y = 82;
    const uint32_t w = info->width > 150 ? info->width - 150 : info->width - 20;
    const uint32_t h = info->height > 120 ? info->height - 132 : info->height / 2U;
    draw_window(info, fb, p, x, y, w, h, "SYSTEM");

    draw_text(info, fb, "MEXOS USER DESKTOP", x + 32, y + 66, 2, p->ink);
    draw_text(info, fb, "FREESTANDING I686 USER PROGRAM", x + 34, y + 108, 1, p->muted);
    draw_text(info, fb, "G: HOME   F: FILES   T: TERMINAL", x + 34, y + 148, 1, p->ink);
    draw_text(info, fb, "S: SHELL  P: SHUTDOWN R: REBOOT", x + 34, y + 176, 1, p->ink);
    draw_text(info, fb, "NO MOUSE SERVER YET. KEYBOARD SESSION IS ACTIVE.", x + 34, y + 224, 1, p->muted);
}

static bool point_in_rect(const int32_t px, const int32_t py,
                        const int32_t x, const int32_t y,
                        const int32_t w, const int32_t h)
{
    return px >= (int32_t)x && px < x + w &&
            py >= (int32_t)y && py < y + h;
}

static void draw_cursor(const struct vesa_mode_info* info, uint8_t* fb,
                        const struct gui_palette* p, const struct gui_state* state)
{
    const int32_t x = state->mouse.x;
    const int32_t y = state->mouse.y;

    for (int32_t row = 0; row < 12; row++)
    {
        const int32_t width = 12 - row;
        fill_rect(info, fb, (uint32_t)x, (uint32_t)(y + row), (uint32_t)width, 1, p->ink);
    }

    stroke_rect(info, fb, (uint32_t)x, (uint32_t)y, 6, 12, p->white);
}

static void draw_desktop(const struct vesa_mode_info* info, uint8_t* fb,
                         const struct gui_palette* p, const struct gui_state* state)
{
    draw_background(info, fb, p);
    draw_topbar(info, fb, p, state);
    draw_dock(info, fb, p, state);

    if (state->view == GUI_VIEW_HOME)
    {
        draw_home(info, fb, p, state);
    }
    else if (state->view == GUI_VIEW_FILES)
    {
        draw_files(info, fb, p, state);
    }
    else if (state->view == GUI_VIEW_TERMINAL)
    {
        draw_terminal(info, fb, p, state);
    }
    else
    {
        draw_system(info, fb, p);
    }

    draw_cursor(info, fb, p, state);
}

static int exec_shell(void)
{
    const char* shell_argv[] = { "sh", NULL };
    return execv("/bin/sh", shell_argv);
}

static int run_program(const char* command)
{
    if (!command || !*command)
    {
        return -1;
    }

    const int child = fork();
    if (child < 0)
    {
        return -1;
    }

    if (child == 0)
    {
        const char* argv[] = { command, NULL };
        if (user_has_slash(command))
        {
            execv(command, argv);
        }
        else
        {
            char path[GUI_PATH_LEN];
            copy_string(path, sizeof(path), "/bin/");
            append_string(path, sizeof(path), command);
            execv(path, argv);
        }
        exit(127);
    }

    int status = 0;
    if (wait(child, &status) < 0)
    {
        return -1;
    }

    return status;
}

static void terminal_ls(struct gui_state* state)
{
    struct fs_dirent entries[GUI_MAX_FILES];
    const int count = readdir(state->cwd, entries, GUI_MAX_FILES);
    if (count < 0)
    {
        log_line(state, "LS FAILED");
        return;
    }

    if (count == 0)
    {
        log_line(state, "EMPTY");
        return;
    }

    for (int i = 0; i < count && i < 5; i++)
    {
        char line[GUI_LOG_LEN];
        copy_string(line, sizeof(line), entries[i].type == FS_ABI_TYPE_DIR ? "[DIR] " : "[BIN] ");
        append_string(line, sizeof(line), entries[i].name);
        log_line(state, line);
    }
}

static void terminal_execute(struct gui_state* state)
{
    char cmd[GUI_CMD_LEN];
    copy_string(cmd, sizeof(cmd), state->command);
    state->command[0] = '\0';
    state->command_len = 0;

    if (!cmd[0])
    {
        return;
    }

    char prompt[GUI_LOG_LEN];
    copy_string(prompt, sizeof(prompt), "> ");
    append_string(prompt, sizeof(prompt), cmd);
    log_line(state, prompt);

    if (user_streq(cmd, "help"))
    {
        log_line(state, "COMMANDS: HELP LS CD PWD RUN SHELL CLEAR");
        log_line(state, "VIEWS: HOME FILES TERM SYSTEM SHUTDOWN");
    }
    else if (user_streq(cmd, "clear"))
    {
        for (int i = 0; i < GUI_LOG_LINES; i++)
        {
            state->log[i][0] = '\0';
        }
    }
    else if (user_streq(cmd, "ls"))
    {
        terminal_ls(state);
    }
    else if (user_streq(cmd, "pwd"))
    {
        update_cwd(state);
        log_line(state, state->cwd);
    }
    else if (starts_with(cmd, "cd "))
    {
        if (chdir(cmd + 3) == 0)
        {
            refresh_files(state);
            log_line(state, "DIRECTORY CHANGED");
        }
        else
        {
            log_line(state, "CD FAILED");
        }
    }
    else if (starts_with(cmd, "run "))
    {
        const int status = run_program(cmd + 4);
        char line[GUI_LOG_LEN];
        char value[16];
        int_to_dec(status, value, sizeof(value));
        copy_string(line, sizeof(line), "EXIT STATUS ");
        append_string(line, sizeof(line), value);
        log_line(state, line);
    }
    else if (user_streq(cmd, "shell"))
    {
        log_line(state, "LEAVING GUI FOR /BIN/SH");
        exec_shell();
    }
    else if (user_streq(cmd, "home"))
    {
        state->view = GUI_VIEW_HOME;
    }
    else if (user_streq(cmd, "files"))
    {
        state->view = GUI_VIEW_FILES;
        refresh_files(state);
    }
    else if (user_streq(cmd, "term") || user_streq(cmd, "terminal"))
    {
        state->view = GUI_VIEW_TERMINAL;
    }
    else if (user_streq(cmd, "system"))
    {
        state->view = GUI_VIEW_SYSTEM;
    }
    else if (user_streq(cmd, "shutdown") || user_streq(cmd, "reboot"))
    {
        shell_exec(cmd);
        log_line(state, "POWER COMMAND SENT");
    }
    else
    {
        if (shell_exec(cmd) == 0)
        {
            log_line(state, "DISPATCHED TO KERNEL SHELL");
        }
        else
        {
            log_line(state, "UNKNOWN COMMAND");
        }
    }
}

static void open_selected_file(struct gui_state* state)
{
    if (state->file_count <= 0 || state->file_index < 0 || state->file_index >= state->file_count)
    {
        return;
    }

    const struct fs_dirent* entry = &state->files[state->file_index];
    if (entry->type == FS_ABI_TYPE_DIR)
    {
        if (chdir(entry->name) == 0)
        {
            refresh_files(state);
            set_status(state, "OPENED DIRECTORY");
        }
        else
        {
            set_status(state, "DIRECTORY OPEN FAILED");
        }
        return;
    }

    if (user_streq(entry->name, "sh"))
    {
        exec_shell();
        return;
    }

    const int status = run_program(entry->name);
    char line[GUI_LOG_LEN];
    char value[16];
    int_to_dec(status, value, sizeof(value));
    copy_string(line, sizeof(line), "PROGRAM EXIT ");
    append_string(line, sizeof(line), value);
    log_line(state, line);
    set_status(state, "PROGRAM LAUNCHED");
}

static void handle_home_key(struct gui_state* state, const unsigned char key)
{
    if (key == KEY_ARROW_RIGHT || key == KEY_ARROW_DOWN || key == 'l' || key == 'j')
    {
        state->launcher_index = (state->launcher_index + 1) % 4;
    }
    else if (key == KEY_ARROW_LEFT || key == KEY_ARROW_UP || key == 'h' || key == 'k')
    {
        state->launcher_index = (state->launcher_index + 3) % 4;
    }
    else if (key == KEY_ENTER)
    {
        if (state->launcher_index == 0)
        {
            state->view = GUI_VIEW_FILES;
            refresh_files(state);
        }
        else if (state->launcher_index == 1)
        {
            state->view = GUI_VIEW_TERMINAL;
        }
        else if (state->launcher_index == 2)
        {
            state->view = GUI_VIEW_SYSTEM;
        }
        else
        {
            exec_shell();
        }
    }
}

static void handle_mouse_click(struct gui_state* state, const struct vesa_mode_info* info, const int32_t mx, const int32_t my)
{
    if (point_in_rect(mx, my, power_shutdown_x(info), GUI_PWR_Y, GUI_PWR_BTN_W, GUI_PWR_BTN_H))
    {
        log_line(state, "SHUTDOWN REQUESTED");
        shell_exec("shutdown");
        return;
    }

    if (point_in_rect(mx, my, power_reboot_x(info), GUI_PWR_Y, GUI_PWR_BTN_W, GUI_PWR_BTN_H))
    {
        log_line(state, "REBOOT REQUESTED");
        shell_exec("reboot");
        return;
    }

    if (point_in_rect(mx, my, 14, 72, 50, 42))
    {
        state->view = GUI_VIEW_HOME;
        return;
    }
    if (point_in_rect(mx, my, 14, 128, 50, 42))
    {
        state->view = GUI_VIEW_FILES;
        refresh_files(state);
        return;
    }
    if (point_in_rect(mx, my, 14, 184, 50, 42))
    {
        state->view = GUI_VIEW_TERMINAL;
        return;
    }
    if (point_in_rect(mx, my, 14, 240, 50, 42))
    {
        state->view = GUI_VIEW_SYSTEM;
        return;
    }

    if (state->view == GUI_VIEW_HOME)
    {
        const uint32_t x = 110, y = 82;
        if (point_in_rect(mx, my, x + 32, y + 140, 200, 86))  { state->launcher_index = 0; handle_home_key(state, KEY_ENTER); }
        else if (point_in_rect(mx, my, x + 260, y + 140, 200, 86)) { state->launcher_index = 1; handle_home_key(state, KEY_ENTER); }
        else if (point_in_rect(mx, my, x + 32, y + 246, 200, 86))  { state->launcher_index = 2; handle_home_key(state, KEY_ENTER); }
        else if (point_in_rect(mx, my, x + 260, y + 246, 200, 86)) { state->launcher_index = 3; handle_home_key(state, KEY_ENTER); }
    }
    else if (state->view == GUI_VIEW_FILES)
    {
        const uint32_t list_top = 82 + 92;
        const uint32_t row_h = 28;

        if (my >= (int32_t)list_top)
        {
            const int row = (my - (int32_t)list_top) / (int32_t)row_h;
            const int index = row;

            if (index >= 0 && index < state->file_count)
            {
                state->file_index = index;
                open_selected_file(state);
            }
        }
    }
}

static void handle_files_key(struct gui_state* state, const unsigned char key)
{
    if ((key == KEY_ARROW_DOWN || key == 'j') && state->file_count > 0)
    {
        state->file_index = (state->file_index + 1) % state->file_count;
    }
    else if ((key == KEY_ARROW_UP || key == 'k') && state->file_count > 0)
    {
        state->file_index = (state->file_index + state->file_count - 1) % state->file_count;
    }
    else if (key == KEY_BACKSPACE || key == 'h')
    {
        if (chdir("..") == 0)
        {
            refresh_files(state);
        }
    }
    else if (key == KEY_ENTER || key == 'l')
    {
        open_selected_file(state);
    }
}

static bool translate_input_key(struct gui_state* state, const unsigned char raw_key, unsigned char* out_key)
{
    if (!out_key)
    {
        return false;
    }

    if (state->escape_state == 1)
    {
        if (raw_key == '[')
        {
            state->escape_state = 2;
            return false;
        }

        state->escape_state = 0;
        *out_key = KEY_ESC;
        return true;
    }

    if (state->escape_state == 2)
    {
        state->escape_state = 0;
        switch (raw_key)
        {
            case 'A': *out_key = KEY_ARROW_UP; return true;
            case 'B': *out_key = KEY_ARROW_DOWN; return true;
            case 'C': *out_key = KEY_ARROW_RIGHT; return true;
            case 'D': *out_key = KEY_ARROW_LEFT; return true;
            default: return false;
        }
    }

    if (raw_key == KEY_ESC)
    {
        state->escape_state = 1;
        return false;
    }

    *out_key = raw_key;
    return true;
}

static void handle_terminal_key(struct gui_state* state, const unsigned char key)
{
    if (key == KEY_ENTER)
    {
        terminal_execute(state);
    }
    else if (key == KEY_BACKSPACE)
    {
        if (state->command_len > 0)
        {
            state->command[--state->command_len] = '\0';
        }
    }
    else if (key >= 0x20 && key < 0x7F && state->command_len + 1 < sizeof(state->command))
    {
        state->command[state->command_len++] = (char)key;
        state->command[state->command_len] = '\0';
    }
}

static void handle_key(struct gui_state* state, const unsigned char key)
{
    if (key == KEY_ESC)
    {
        state->view = GUI_VIEW_HOME;
        return;
    }

    if (key == KEY_TAB)
    {
        state->view = (enum gui_view)(((int)state->view + 1) % GUI_VIEW_COUNT);
        if (state->view == GUI_VIEW_FILES)
        {
            refresh_files(state);
        }
        return;
    }

    if (state->view == GUI_VIEW_TERMINAL)
    {
        handle_terminal_key(state, key);
        return;
    }

    if (key == '1' || key == 'g' || key == 'G')
    {
        state->view = GUI_VIEW_HOME;
        return;
    }
    if (key == '2' || key == 'f' || key == 'F')
    {
        state->view = GUI_VIEW_FILES;
        refresh_files(state);
        return;
    }
    if (key == '3' || key == 't' || key == 'T')
    {
        state->view = GUI_VIEW_TERMINAL;
        return;
    }
    if (key == '4')
    {
        state->view = GUI_VIEW_SYSTEM;
        return;
    }
    if (key == 's' || key == 'S')
    {
        exec_shell();
        return;
    }
    if (key == 'p' || key == 'P')
    {
        shell_exec("shutdown");
        return;
    }
    if (key == 'r' || key == 'R')
    {
        shell_exec("reboot");
        return;
    }

    if (state->view == GUI_VIEW_HOME)
    {
        handle_home_key(state, key);
    }
    else if (state->view == GUI_VIEW_FILES)
    {
        handle_files_key(state, key);
    }
}

static void init_state(struct gui_state* state)
{
    user_memset(state, 0, sizeof(*state));
    state->view = GUI_VIEW_HOME;
    state->launcher_index = 0;
    state->running = true;
    state->escape_state = 0;
    state->mouse_was_down = false;
    update_cwd(state);
    refresh_files(state);
    set_status(state, "READY");
    log_line(state, "MEXOS GUI READY");
    log_line(state, "TYPE HELP");
}

int main(const int argc, char** argv)
{
    UNUSED(argc, "Currently not used, check again!");
    UNUSED(argv, "Currently not used, check again!");

    struct vesa_mode_info info;
    user_memset(&info, 0, sizeof(info));

    uint8_t* fb = mmap_fb(&info);
    if (!fb)
    {
        user_println("gui: framebuffer not available");
        user_println("gui: falling back to /bin/sh");
        exec_shell();
        return 1;
    }

    if (info.bpp != 24 && info.bpp != 32)
    {
        user_println("gui: unsupported framebuffer depth");
        exec_shell();
        return 1;
    }

    const uint32_t fb_bytes = info.pitch * info.height;
    uint8_t* backbuffer = (uint8_t*)mmap_anon(fb_bytes);
    uint8_t* draw_target = backbuffer ? backbuffer : fb;

    if (!backbuffer)
    {
        user_println("gui: backbuffer allocation failed, drawing direct (may cause flickering)");
    }

    struct gui_palette palette;
    make_palette(&info, &palette);

    struct gui_state state;
    init_state(&state);

    user_println("[gui] user-space desktop started");

    struct rtc_time last_check;
    user_memset(&last_check, 0, sizeof(last_check));

    bool dirty = true;
    while (state.running)
    {
        struct rtc_time now_clock;
        if (gettime(&now_clock) == 0 &&
            (now_clock.second != last_check.second ||
             now_clock.minute != last_check.minute ||
             now_clock.hour != last_check.hour))
        {
            last_check = now_clock;
            dirty = true;
        }

        if (dirty)
        {
            //draw_desktop(&info, fb, &palette, &state);
            draw_desktop(&info, draw_target, &palette, &state);
            if (draw_target != fb)
            {
                user_memcpy(fb, draw_target, fb_bytes);
            }
            dirty = false;
        }

        struct mouse_state mstate;
        poll_mouse(&mstate);
        if (mstate.x != state.mouse.x || mstate.y != state.mouse.y || mstate.buttons != state.mouse.buttons)
        {
            state.mouse = mstate;
            dirty = true;
        }

        const bool mouse_down_now = (state.mouse.buttons & MOUSE_LEFT_BUTTON) != 0;
        if (mouse_down_now && !state.mouse_was_down)
        {
            handle_mouse_click(&state, &info, state.mouse.x, state.mouse.y);
            dirty = true;
        }
        state.mouse_was_down = mouse_down_now;

        unsigned char raw_key = 0;
        const int poll_result = poll_key(&raw_key);
        if (poll_result < 0)
        {
            set_status(&state, "INPUT ERROR");
            dirty = true;
            yield();
            continue;
        }

        if (poll_result == 0)
        {
            yield();
            continue;
        }

        if (raw_key == '\r')
        {
            raw_key = '\n';
        }

        unsigned char key = 0;
        if (!translate_input_key(&state, raw_key, &key))
        {
            dirty = true;
            continue;
        }

        set_key_status(&state, key);
        handle_key(&state, key);
        dirty = true;
    }

    exec_shell();
    return 0;
}