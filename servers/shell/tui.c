#include "tui.h"
#include "../console/vterm.h"
#include "../console/console.h"
#include "../../shared/log.h"
#include "../../servers/vfs/fs.h"
#include "../input/keyboard.h"
#include "editor.h"
#include "../../shared/string.h"
#include "sched/sched.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "sys/timer.h"
#include "sys/sysmon.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define CHAR_HLINE_SINGLE     '-'
#define CHAR_VLINE_SINGLE     '|'
#define CHAR_TL_CORNER_SINGLE '+'
#define CHAR_TR_CORNER_SINGLE '+'
#define CHAR_BL_CORNER_SINGLE '+'
#define CHAR_BR_CORNER_SINGLE '+'
#define CHAR_T_JUNCTION       '+'
#define CHAR_B_JUNCTION       '+'
#define CHAR_L_JUNCTION       '+'
#define CHAR_R_JUNCTION       '+'
#define CHAR_CROSS            '+'

#define CHAR_HLINE_DOUBLE     '='
#define CHAR_VLINE_DOUBLE     '|'
#define CHAR_TL_CORNER_DOUBLE '+'
#define CHAR_TR_CORNER_DOUBLE '+'
#define CHAR_BL_CORNER_DOUBLE '+'
#define CHAR_BR_CORNER_DOUBLE '+'

#define CHAR_BLOCK_FULL  '#'
#define CHAR_BLOCK_EMPTY ' '

static struct tui_panel panels[TUI_MAX_PANELS];
static uint8_t panel_count = 0;

void tui_init(void)
{
    memset(panels, 0, sizeof(panels));
    panel_count = 0;
}

int tui_create_panel(const uint8_t x, const uint8_t y, const uint8_t width, const uint8_t height, const char* title)
{
    if (panel_count >= TUI_MAX_PANELS)
    {
        return -1;
    }

    struct tui_panel* panel = &panels[panel_count];
    panel->x = x;
    panel->y = y;
    panel->width = width;
    panel->height = height;
    panel->border_style = TUI_BORDER_SINGLE;
    panel->fg_color = VGA_LIGHT_GREY;
    panel->bg_color = VGA_BLACK;
    panel->visible = true;

    if (title)
    {
        strncpy(panel->title, title, sizeof(panel->title) - 1);
        panel->title[sizeof(panel->title) - 1] = '\0';
    }
    else
    {
        panel->title[0] = '\0';
    }

    return panel_count++;
}

void tui_set_panel_colors(const int panel_id, const uint8_t fg, const uint8_t bg)
{
    if (panel_id < 0 || panel_id >= panel_count)
    {
        return;
    }

    panels[panel_id].fg_color = fg;
    panels[panel_id].bg_color = bg;
}

static void tui_put_char_at(const uint8_t x, const uint8_t y, const char c, const uint8_t fg, const uint8_t bg)
{
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT)
    {
        return;
    }

    uint16_t* vga = (uint16_t*)0xB8000;
    const uint8_t color = (bg << 4) | fg;
    vga[y * VGA_WIDTH + x] = ((uint16_t)color << 8) | c;
}

static void tui_write_string_at(const uint8_t x, const uint8_t y, const char* str, const uint8_t fg, const uint8_t bg)
{
    for (uint8_t i = 0; str[i] && (x + i) < VGA_WIDTH; i++)
    {
        tui_put_char_at(x + i, y, str[i], fg, bg);
    }
}

void tui_draw_hline(const uint8_t x, const uint8_t y, const uint8_t length, const uint8_t style)
{
    const char c = (style == TUI_BORDER_DOUBLE) ? CHAR_HLINE_DOUBLE : CHAR_HLINE_SINGLE;

    for (uint8_t i = 0; i < length; i++)
    {
        tui_put_char_at(x + i, y, c, VGA_LIGHT_GREY, VGA_BLACK);
    }
}

void tui_draw_vline(const uint8_t x, const uint8_t y, const uint8_t length, const uint8_t style)
{
    const char c = (style == TUI_BORDER_DOUBLE) ? CHAR_VLINE_DOUBLE : CHAR_VLINE_SINGLE;

    for (uint8_t i = 0; i < length; i++)
    {
        tui_put_char_at(x, y + i, c, VGA_LIGHT_GREY, VGA_BLACK);
    }
}

void tui_draw_panel(const int panel_id)
{
    if (panel_id < 0 || panel_id >= panel_count)
    {
        return;
    }

    const struct tui_panel* p = &panels[panel_id];

    if (!p->visible)
    {
        return;
    }

    const uint8_t fg = p->fg_color;
    const uint8_t bg = p->bg_color;

    const char tl = (p->border_style == TUI_BORDER_DOUBLE) ? CHAR_TL_CORNER_DOUBLE : CHAR_TL_CORNER_SINGLE;
    const char tr = (p->border_style == TUI_BORDER_DOUBLE) ? CHAR_TR_CORNER_DOUBLE : CHAR_TR_CORNER_SINGLE;
    const char bl = (p->border_style == TUI_BORDER_DOUBLE) ? CHAR_BL_CORNER_DOUBLE : CHAR_BL_CORNER_SINGLE;
    const char br = (p->border_style == TUI_BORDER_DOUBLE) ? CHAR_BR_CORNER_DOUBLE : CHAR_BR_CORNER_SINGLE;

    tui_put_char_at(p->x, p->y, tl, fg, bg);
    tui_put_char_at(p->x + p->width - 1, p->y, tr, fg, bg);
    tui_put_char_at(p->x, p->y + p->height - 1, bl, fg, bg);
    tui_put_char_at(p->x + p->width - 1, p->y + p->height - 1, br, fg, bg);

    tui_draw_hline(p->x + 1, p->y, p->width - 2, p->border_style);
    tui_draw_hline(p->x + 1, p->y + p->height - 1, p->width - 2, p->border_style);

    tui_draw_vline(p->x, p->y + 1, p->height - 2, p->border_style);
    tui_draw_vline(p->x + p->width - 1, p->y + 1, p->height - 2, p->border_style);

    if (p->title[0] != '\0')
    {
        const uint8_t title_len = strlen(p->title);
        if (title_len > 0 && title_len < p->width - 4)
        {
            const uint8_t title_x = p->x + 2;
            tui_put_char_at(title_x - 1, p->y, ' ', fg, bg);
            tui_write_string_at(title_x, p->y, p->title, VGA_WHITE, bg);
            tui_put_char_at(title_x + title_len, p->y, ' ', fg, bg);
        }
    }
}

void tui_clear_panel(const int panel_id)
{
    if (panel_id < 0 || panel_id >= panel_count)
    {
        return;
    }

    const struct tui_panel* p = &panels[panel_id];

    for (uint8_t y = 1; y < p->height - 1; y++)
    {
        for (uint8_t x = 1; x < p->width - 1; x++)
        {
            tui_put_char_at(p->x + x, p->y + y, ' ', p->fg_color, p->bg_color);
        }
    }
}

void tui_panel_write(const int panel_id, const uint8_t x, const uint8_t y, const char* text)
{
    if (panel_id < 0 || panel_id >= panel_count)
    {
        return;
    }

    const struct tui_panel* p = &panels[panel_id];

    const uint8_t screen_x = p->x + x + 1;
    const uint8_t screen_y = p->y + y + 1;

    if (screen_x >= p->x + p->width - 1 || screen_y >= p->y + p->height - 1)
    {
        return;
    }

    tui_write_string_at(screen_x, screen_y, text, p->fg_color, p->bg_color);
}

void tui_draw_progress_bar(const struct tui_progress_bar* bar)
{
    if (!bar || bar->value > 100)
    {
        return;
    }

    if (bar->label[0] != '\0')
    {
        tui_write_string_at(bar->x, bar->y, bar->label, bar->fg_color, bar->bg_color);
    }

    const uint8_t filled = (bar->width * bar->value) / 100;

    tui_put_char_at(bar->x, bar->y, '[', bar->fg_color, bar->bg_color);

    for (uint8_t i = 0; i < bar->width; i++)
    {
        const char c = (i < filled) ? CHAR_BLOCK_FULL : CHAR_BLOCK_EMPTY;
        tui_put_char_at(bar->x + i + 1, bar->y, c, bar->fg_color, bar->bg_color);
    }

    tui_put_char_at(bar->x + bar->width + 1, bar->y, ']', bar->fg_color, bar->bg_color);

    char percent[8];
    int_to_str_pad(bar->value, percent, 1);
    strcat(percent, "%");
    tui_write_string_at(bar->x + bar->width + 3, bar->y, percent, bar->fg_color, bar->bg_color);
}

void tui_draw_status_bar(const char* text)
{
    for (uint8_t x = 0; x < VGA_WIDTH; x++)
    {
        tui_put_char_at(x, VGA_HEIGHT - 1, ' ', VGA_BLACK, VGA_LIGHT_GREY);
    }

    tui_write_string_at(1, VGA_HEIGHT - 1, text, VGA_BLACK, VGA_LIGHT_GREY);
}

static void tui_write_number_at(const uint8_t x, const uint8_t y, const uint32_t num, const uint8_t fg, const uint8_t bg)
{
    char buf[16];
    int_to_str_pad((int)num, buf, 1);
    tui_write_string_at(x, y, buf, fg, bg);
}

void tui_draw_dashboard(void)
{
    console_clear();

    const int main_panel = tui_create_panel(0, 0, VGA_WIDTH, VGA_HEIGHT - 1, " mexOS System Dashboard ");
    tui_set_panel_colors(main_panel, VGA_LIGHT_GREY, VGA_BLACK);
    tui_draw_panel(main_panel);

    tui_draw_hline(1, 4, VGA_WIDTH - 2, TUI_BORDER_SINGLE);
    tui_draw_hline(1, 14, VGA_WIDTH - 2, TUI_BORDER_SINGLE);
    tui_draw_hline(1, 19, VGA_WIDTH - 2, TUI_BORDER_SINGLE);

    tui_panel_write(main_panel, 1, 0, "CPU Usage:");
    tui_panel_write(main_panel, 40, 0, "Uptime:");
    tui_panel_write(main_panel, 1, 1, "Memory:");
    tui_panel_write(main_panel, 40, 1, "Tasks:");
    tui_panel_write(main_panel, 1, 2, "Heap Free:");
    tui_panel_write(main_panel, 40, 2, "PMM Free:");

    tui_panel_write(main_panel, 1, 4, "PID  Name       State     CPU%  Stack");

    tui_panel_write(main_panel, 1, 14, "Memory Details:");

    tui_panel_write(main_panel, 1, 19, "System Log (last 3 entries):");

    tui_draw_status_bar(" ESC:Exit  F2:Shell  F3:Tests  Arrow Keys:Scroll  Auto-refresh:500ms ");

    tui_update_dashboard();
}

void tui_update_dashboard(void)
{
    if (panel_count == 0)
    {
        return;
    }

    const int main_panel = 0;

    const uint32_t uptime_ms = timer_get_ticks() * 10;
    const uint32_t uptime_sec = uptime_ms / 1000;
    const uint32_t uptime_min = uptime_sec / 60;
    const uint32_t uptime_hour = uptime_min / 60;

    const size_t heap_used = heap_get_used();
    const size_t heap_free = heap_get_free();
    const uint32_t heap_total = heap_used + heap_free;
    const uint32_t mem_percent = (heap_used * 100) / heap_total;

    const uint32_t pmm_total = pmm_get_block_count();
    const uint32_t pmm_free = pmm_get_free_block_count();

    struct task* current = sched_get_current();
    const struct task* idle = sched_get_idle_task();

    const uint32_t total_ticks = sched_get_total_ticks();

    uint32_t cpu_percent = 0;
    if (idle && total_ticks > 0)
    {
        const uint32_t idle_percent = (idle->cpu_ticks * 100) / total_ticks;
        cpu_percent = (idle_percent < 100) ? (100 - idle_percent) : 0;
    }

    struct tui_progress_bar cpu_bar;
    cpu_bar.x = 13;
    cpu_bar.y = 1;
    cpu_bar.width = 20;
    cpu_bar.value = cpu_percent;
    cpu_bar.fg_color = (cpu_percent > 80) ? VGA_LIGHT_RED : VGA_LIGHT_GREEN;
    cpu_bar.bg_color = VGA_BLACK;
    cpu_bar.label[0] = '\0';
    tui_draw_progress_bar(&cpu_bar);

    struct tui_progress_bar mem_bar;
    mem_bar.x = 13;
    mem_bar.y = 2;
    mem_bar.width = 20;
    mem_bar.value = mem_percent;
    mem_bar.fg_color = (mem_percent > 80) ? VGA_LIGHT_RED : VGA_LIGHT_CYAN;
    mem_bar.bg_color = VGA_BLACK;
    mem_bar.label[0] = '\0';
    tui_draw_progress_bar(&mem_bar);

    char uptime_str[32];
    char h[8], m[8], s[8];
    int_to_str_pad((int)uptime_hour, h, 1);
    int_to_str_pad((int)uptime_min % 60, m, 1);
    int_to_str_pad((int)uptime_sec % 60, s, 1);
    strcpy(uptime_str, h);
    strcat(uptime_str, ":");
    if ((uptime_min % 60) < 10) strcat(uptime_str, "0");
    strcat(uptime_str, m);
    strcat(uptime_str, ":");
    if ((uptime_sec % 60) < 10) strcat(uptime_str, "0");
    strcat(uptime_str, s);
    tui_panel_write(main_panel, 48, 0, uptime_str);

    uint32_t task_count = 0;
    const struct task* t = sched_get_task_list();
    while (t)
    {
        task_count++;
        t = t->next;
    }
    tui_write_number_at(48, 2, task_count, VGA_LIGHT_GREY, VGA_BLACK);

    char heap_str[32];
    int_to_str_pad((int)heap_free / 1024, heap_str, 1);
    strcat(heap_str, " KB");
    tui_panel_write(main_panel, 12, 2, heap_str);

    char pmm_str[32];
    int_to_str_pad((int)pmm_free * 4, pmm_str, 1);
    strcat(pmm_str, " KB");
    tui_panel_write(main_panel, 51, 2, pmm_str);

    t = sched_get_task_list();
    uint8_t row = 5;
    const uint8_t max_tasks = 8;
    uint8_t task_num = 0;

    while (t && task_num < max_tasks)
    {
        char line[80];
        char pid_str[8], cpu_str[8];

        int_to_str_pad(t->pid, pid_str, 1);

        uint32_t task_cpu = 0;
        if (total_ticks > 0)
        {
            task_cpu = (t->cpu_ticks * 100) / total_ticks;
        }
        int_to_str_pad((int)task_cpu, cpu_str, 1);

        const char* state_str;
        switch (t->state)
        {
            case TASK_RUNNING: state_str = "RUNNING "; break;
            case TASK_READY:   state_str = "READY   "; break;
            case TASK_BLOCKED: state_str = "BLOCKED "; break;
            case TASK_ZOMBIE:  state_str = "ZOMBIE  "; break;
            default:           state_str = "UNKNOWN "; break;
        }

        strcpy(line, " ");
        if (t->pid < 10) strcat(line, " ");
        strcat(line, pid_str);
        strcat(line, "   ");

        if (t->pid == 0)
        {
            strcat(line, "idle     ");
        }
        else if (t->pid == 1)
        {
            strcat(line, "init     ");
        }
        else if (t->pid == 2)
        {
            strcat(line, "shell    ");
        }
        else
        {
            strcat(line, "task");
            strcat(line, pid_str);
            strcat(line, "    ");
        }

        strcat(line, state_str);
        strcat(line, " ");
        if (task_cpu < 10) strcat(line, " ");
        strcat(line, cpu_str);
        strcat(line, "%  ");

        if (t->kernel_stack)
        {
            strcat(line, "4KB");
        }

        tui_panel_write(main_panel, 0, row, line);

        row++;
        task_num++;
        t = t->next;
    }

    while (row < 13)
    {
        tui_panel_write(main_panel, 0, row, "                                                  ");
        row++;
    }

    uint32_t free_blocks = 0;
    uint32_t largest_free = 0;
    heap_get_fragmentation(&free_blocks, &largest_free);

    char mem_detail[80];
    char num_str[16];

    strcpy(mem_detail, "  Heap: ");
    int_to_str_pad((int)heap_total / 1024, num_str, 1);
    strcat(mem_detail, num_str);
    strcat(mem_detail, " KB total, ");
    int_to_str_pad((int)heap_used / 1024, num_str, 1);
    strcat(mem_detail, num_str);
    strcat(mem_detail, " KB used, ");
    int_to_str_pad((int)free_blocks, num_str, 1);
    strcat(mem_detail, num_str);
    strcat(mem_detail, " blocks");
    tui_panel_write(main_panel, 0, 15, mem_detail);

    strcpy(mem_detail, "  PMM:  ");
    int_to_str_pad((int)pmm_total * 4 / 1024, num_str, 1);
    strcat(mem_detail, num_str);
    strcat(mem_detail, " MB total, ");
    int_to_str_pad((int)(pmm_total - pmm_free) * 4, num_str, 1);
    strcat(mem_detail, num_str);
    strcat(mem_detail, " KB used");
    tui_panel_write(main_panel, 0, 16, mem_detail);

    strcpy(mem_detail, "  Largest free block: ");
    int_to_str_pad((int)largest_free / 1024, num_str, 1);
    strcat(mem_detail, num_str);
    strcat(mem_detail, " KB");
    tui_panel_write(main_panel, 0, 17, mem_detail);

    tui_panel_write(main_panel, 0, 20, "  System running normally. Press any key to exit.");
}

void tui_show_log_viewer(void)
{
    console_clear();
    tui_init();

    const int log_panel = tui_create_panel(0, 0, VGA_WIDTH, VGA_HEIGHT - 1, " System Log Viewer ");
    tui_set_panel_colors(log_panel, VGA_LIGHT_GREY, VGA_BLACK);
    tui_draw_panel(log_panel);

    tui_panel_write(log_panel, 1, 0, "Time    Level  Message");
    tui_draw_hline(1, 2, VGA_WIDTH - 2, TUI_BORDER_SINGLE);

    const uint32_t log_count = log_get_count();
    const uint32_t max_display = 18;
    const uint32_t start_idx = (log_count > max_display) ? (log_count - max_display) : 0;

    for (uint32_t i = 0; i < max_display && (start_idx + i) < log_count; i++)
    {
        const struct log_entry* entry = log_get_entry(start_idx + i);
        if (!entry)
        {
            continue;
        }

        char line[80];
        char time_str[16];

        const uint32_t time_sec = entry->timestamp / 100;
        const uint32_t time_ms = (entry->timestamp % 100) * 10;
        int_to_str_pad((int)time_sec, time_str, 1);
        strcat(time_str, ".");
        char ms_str[8];
        int_to_str_pad((int)time_ms / 100, ms_str, 1);
        strcat(time_str, ms_str);

        strcpy(line, time_str);
        while (strlen(line) < 8)
        {
            strcat(line, " ");
        }

        const char* level_str;
        switch (entry->level)
        {
            case LOG_LEVEL_DEBUG: level_str = "DBG "; break;
            case LOG_LEVEL_INFO:  level_str = "INF "; break;
            case LOG_LEVEL_WARN:  level_str = "WRN "; break;
            case LOG_LEVEL_ERROR: level_str = "ERR "; break;
            default:              level_str = "??? "; break;
        }
        strcat(line, level_str);
        strcat(line, " ");

        strncat(line, entry->message, 60);

        tui_panel_write(log_panel, 0, i + 3, line);
    }

    char status[80];
    strcpy(status, "Showing ");
    char num_str[16];
    int_to_str_pad((int)log_count, num_str, 1);
    strcat(status, num_str);
    strcat(status, " log entries");
    tui_draw_status_bar(status);
}

void tui_show_file_browser(const char* path)
{
    console_clear();
    tui_init();

    const int file_panel = tui_create_panel(0, 0, VGA_WIDTH, VGA_HEIGHT - 1, " File Browser ");
    tui_set_panel_colors(file_panel, VGA_LIGHT_GREY, VGA_BLACK);
    tui_draw_panel(file_panel);

    char cwd[256];
    strcpy(cwd, "Current: ");
    strcat(cwd, fs_get_cwd());
    tui_panel_write(file_panel, 1, 0, cwd);
    tui_draw_hline(1, 2, VGA_WIDTH - 2, TUI_BORDER_SINGLE);

    char buffer[1024];
    const int ret = fs_list_dir(path ? path : ".", buffer, sizeof(buffer));

    if (ret == FS_ERR_NOT_FOUND)
    {
        tui_panel_write(file_panel, 1, 4, "Directory not found");
    }
    else if (ret == FS_ERR_NOT_DIR)
    {
        tui_panel_write(file_panel, 1, 4, "Not a directory");
    }
    else if (ret == 0)
    {
        tui_panel_write(file_panel, 1, 4, "(empty directory)");
    }
    else
    {
        const char* line = buffer;
        uint8_t row = 3;
        while (*line && row < 20)
        {
            char display_line[80];
            uint8_t i = 0;
            while (*line && *line != '\n' && i < 70)
            {
                display_line[i++] = *line++;
            }
            display_line[i] = '\0';
            if (*line == '\n')
            {
                line++;
            }

            tui_panel_write(file_panel, 1, row++, display_line);
        }
    }

    tui_draw_status_bar(" Arrow Keys:Navigate  Enter:Open  ESC:Back  mkdir/touch/rm in shell ");
}

void tui_show_task_manager(void)
{
    console_clear();
    tui_init();

    const int task_panel = tui_create_panel(0, 0, VGA_WIDTH, VGA_HEIGHT - 1, " Task Manager ");
    tui_set_panel_colors(task_panel, VGA_LIGHT_GREY, VGA_BLACK);
    tui_draw_panel(task_panel);

    tui_panel_write(task_panel, 1, 0, "PID  Name        State     Priority  CPU%   Stack");
    tui_draw_hline(1, 2, VGA_WIDTH - 2, TUI_BORDER_SINGLE);

    const uint32_t total_ticks = sched_get_total_ticks();
    const struct task* t = sched_get_task_list();
    uint8_t row = 3;
    uint32_t task_count = 0;

    while (t && row < 20)
    {
        char line[80];
        char pid_str[8], cpu_str[8], pri_str[8];

        int_to_str_pad(t->pid, pid_str, 1);
        int_to_str_pad(t->priority, pri_str, 1);

        uint32_t cpu_percent = 0;
        if (total_ticks > 0)
        {
            cpu_percent = (t->cpu_ticks * 100) / total_ticks;
        }
        int_to_str_pad((int)cpu_percent, cpu_str, 1);

        const char* state_str;
        uint8_t color = VGA_LIGHT_GREY;
        switch (t->state)
        {
            case TASK_RUNNING: state_str = "RUNNING "; color = VGA_LIGHT_GREEN; break;
            case TASK_READY:   state_str = "READY   "; break;
            case TASK_BLOCKED: state_str = "BLOCKED "; color = VGA_YELLOW; break;
            case TASK_ZOMBIE:  state_str = "ZOMBIE  "; color = VGA_LIGHT_RED; break;
            default:           state_str = "UNKNOWN "; break;
        }

        strcpy(line, " ");
        if (t->pid < 10)
        {
            strcat(line, " ");
        }
        strcat(line, pid_str);
        strcat(line, "   ");

        if (t->pid == 0)
        {
            strcat(line, "idle      ");
        }
        else if (t->pid == 1)
        {
            strcat(line, "init      ");
        }
        else if (t->pid == 2)
        {
            strcat(line, "shell     ");
        }
        else
        {
            strcat(line, "task");
            strcat(line, pid_str);
            strcat(line, "     ");
        }

        strcat(line, state_str);
        strcat(line, "  ");
        strcat(line, pri_str);
        strcat(line, "         ");
        if (cpu_percent < 10)
        {
            strcat(line, " ");
        }
        strcat(line, cpu_str);
        strcat(line, "%    ");

        if (t->kernel_stack)
        {
            strcat(line, "4KB");
        }
        else
        {
            strcat(line, "N/A");
        }

        tui_write_string_at(2, row, line, color, VGA_BLACK);

        row++;
        task_count++;
        t = t->next;
    }

    char status[80];
    strcpy(status, "Total tasks: ");
    char num_str[16];
    int_to_str_pad((int)task_count, num_str, 1);
    strcat(status, num_str);
    strcat(status, "  |  k:Kill  r:Renice  ESC:Back");
    tui_draw_status_bar(status);
}

void tui_show_memory_monitor(void)
{
    console_clear();
    tui_init();

    const int mem_panel = tui_create_panel(0, 0, VGA_WIDTH, VGA_HEIGHT - 1, " Memory Monitor ");
    tui_set_panel_colors(mem_panel, VGA_LIGHT_GREY, VGA_BLACK);
    tui_draw_panel(mem_panel);

    tui_panel_write(mem_panel, 1, 0, "Kernel Heap:");
    tui_draw_hline(1, 2, VGA_WIDTH - 2, TUI_BORDER_SINGLE);

    const size_t heap_used = heap_get_used();
    const size_t heap_free = heap_get_free();
    const uint32_t heap_total = heap_used + heap_free;

    char line[80];
    char num_str[16];

    strcpy(line, "  Total:  ");
    int_to_str_pad((int)heap_total / 1024, num_str, 1);
    strcat(line, num_str);
    strcat(line, " KB (");
    int_to_str_pad((int)heap_total, num_str, 1);
    strcat(line, num_str);
    strcat(line, " bytes)");
    tui_panel_write(mem_panel, 0, 3, line);

    strcpy(line, "  Used:   ");
    int_to_str_pad((int)heap_used / 1024, num_str, 1);
    strcat(line, num_str);
    strcat(line, " KB (");
    int_to_str_pad((heap_used * 100) / heap_total, num_str, 1);
    strcat(line, num_str);
    strcat(line, "%)");
    tui_panel_write(mem_panel, 0, 4, line);

    strcpy(line, "  Free:   ");
    int_to_str_pad((int)heap_free / 1024, num_str, 1);
    strcat(line, num_str);
    strcat(line, " KB (");
    int_to_str_pad((heap_free * 100) / heap_total, num_str, 1);
    strcat(line, num_str);
    strcat(line, "%)");
    tui_panel_write(mem_panel, 0, 5, line);

    uint32_t free_blocks = 0;
    uint32_t largest_free = 0;
    heap_get_fragmentation(&free_blocks, &largest_free);

    strcpy(line, "  Free blocks: ");
    int_to_str_pad((int)free_blocks, num_str, 1);
    strcat(line, num_str);
    tui_panel_write(mem_panel, 0, 6, line);

    strcpy(line, "  Largest block: ");
    int_to_str_pad((int)largest_free / 1024, num_str, 1);
    strcat(line, num_str);
    strcat(line, " KB");
    tui_panel_write(mem_panel, 0, 7, line);

    tui_panel_write(mem_panel, 1, 9, "Physical Memory:");
    tui_draw_hline(1, 11, VGA_WIDTH - 2, TUI_BORDER_SINGLE);

    const uint32_t pmm_total = pmm_get_block_count();
    const uint32_t pmm_used = pmm_get_used_block_count();
    const uint32_t pmm_free = pmm_get_free_block_count();

    strcpy(line, "  Total blocks:  ");
    int_to_str_pad((int)pmm_total, num_str, 1);
    strcat(line, num_str);
    strcat(line, " (");
    int_to_str_pad((int)pmm_total * 4 / 1024, num_str, 1);
    strcat(line, num_str);
    strcat(line, " MB)");
    tui_panel_write(mem_panel, 0, 12, line);

    strcpy(line, "  Used blocks:   ");
    int_to_str_pad((int)pmm_used, num_str, 1);
    strcat(line, num_str);
    strcat(line, " (");
    int_to_str_pad((int)pmm_used * 4, num_str, 1);
    strcat(line, num_str);
    strcat(line, " KB)");
    tui_panel_write(mem_panel, 0, 13, line);

    strcpy(line, "  Free blocks:   ");
    int_to_str_pad((int)pmm_free, num_str, 1);
    strcat(line, num_str);
    strcat(line, " (");
    int_to_str_pad((int)pmm_free * 4, num_str, 1);
    strcat(line, num_str);
    strcat(line, " KB)");
    tui_panel_write(mem_panel, 0, 14, line);

    tui_draw_status_bar(" d:Defragment Heap  ESC:Back ");
}

void tui_show_editor(void)
{
    console_clear();
    tui_init();

    const int editor_panel = tui_create_panel(0, 0, VGA_WIDTH, VGA_HEIGHT - 1, " Text Editor ");
    tui_set_panel_colors(editor_panel, VGA_LIGHT_GREY, VGA_BLACK);
    tui_draw_panel(editor_panel);

    tui_panel_write(editor_panel, 1, 0, "Quick File Editor - Enter filename to edit:");
    tui_draw_hline(1, 2, VGA_WIDTH - 2, TUI_BORDER_SINGLE);

    tui_panel_write(editor_panel, 1, 4, "Recent files:");

    char buffer[512];
    const int ret = fs_list_dir(".", buffer, sizeof(buffer));

    if (ret > 0)
    {
        const char* line = buffer;
        uint8_t row = 5;
        uint8_t count = 0;

        while (*line && row < 15 && count < 8)
        {
            char display_line[80];

            if (*line != '[')
            {
                uint8_t i = 0;
                strcpy(display_line, "  ");
                while (*line && *line != '\n' && i < 60)
                {
                    display_line[i + 2] = *line++;
                    i++;
                }
                display_line[i + 2] = '\0';

                tui_panel_write(editor_panel, 1, row++, display_line);
                count++;
            }

            while (*line && *line != '\n') line++;
            if (*line == '\n') line++;
        }
    }
    else
    {
        tui_panel_write(editor_panel, 1, 5, "  (no files in current directory)");
    }

    tui_panel_write(editor_panel, 1, 16, "Commands:");
    tui_panel_write(editor_panel, 1, 17, "  e - Open text editor (new/existing file)");
    tui_panel_write(editor_panel, 1, 18, "  b - Start BASIC interpreter");
    tui_panel_write(editor_panel, 1, 19, "  ESC - Return to menu");

    tui_draw_status_bar(" Press 'e' for text editor, 'b' for BASIC, ESC to go back ");
}

void tui_run_app(void)
{
    uint8_t current_screen = 0;
    const uint8_t num_screens = 5;
    const char* screen_names[] = {
        "Dashboard",
        "Log Viewer",
        "File Browser",
        "Task Manager",
        "Memory Monitor",
        "Editor"
    };

    while (1)
    {
        //used 1,2,3,4 key to swich screen
        switch (current_screen)
        {
            case 0:
                tui_draw_dashboard();
                break;
            case 1:
                tui_show_log_viewer();
                break;
            case 2:
                tui_show_file_browser(".");
                break;
            case 3:
                tui_show_task_manager();
                break;
            case 4:
                tui_show_memory_monitor();
                break;
            case 5:
                tui_show_editor();
                break;
        }

        uint32_t last_update = timer_get_ticks();
        while (1)
        {
            const unsigned char c = keyboard_getchar();

            if (c == 27)
            {
                console_clear();
                return;
            }
            else if (c == KEY_ARROW_LEFT && current_screen > 0)
            {
                current_screen--;
                break;
            }
            else if (c == KEY_ARROW_RIGHT && current_screen < num_screens - 1)
            {
                current_screen++;
                break;
            }
            else if (c >= '1' && c <= '6')
            {
                current_screen = c - '1';
                break;
            }

            if (current_screen == 5)
            {
                if (c == 'e' || c == 'E')
                {
                    console_clear();
                    console_write("Enter filename to edit (or press Enter for new file): ");

                    char filename[128];
                    uint32_t pos = 0;
                    memset(filename, 0, sizeof(filename));

                    while (1)
                    {
                        const char ch = keyboard_getchar();

                        if (ch == '\n')
                        {
                            console_putchar('\n');
                            break;
                        }
                        else if (ch == '\b')
                        {
                            if (pos > 0)
                            {
                                pos--;
                                console_putchar('\b');
                                console_putchar(' ');
                                console_putchar('\b');
                            }
                        }
                        else if (ch >= 0x20 && ch < 0x7F && pos < 127)
                        {
                            filename[pos++] = ch;
                            console_putchar(ch);
                        }
                    }

                    filename[pos] = '\0';

                    if (pos == 0)
                    {
                        strcpy(filename, "untitled.txt");
                    }

                    uint8_t mode = EDITOR_MODE_TEXT;
                    const char* ext = filename + strlen(filename);
                    while (ext > filename && *ext != '.') ext--;

                    if (strcmp(ext, ".bas") == 0 || strcmp(ext, ".BAS") == 0)
                    {
                        mode = EDITOR_MODE_BASIC;
                    }

                    if (editor_open(filename, mode) == 0)
                    {
                        editor_run();
                    }

                    break;
                }
                else if (c == 'b' || c == 'B')
                {
                    if (editor_open("untitled.bas", EDITOR_MODE_BASIC) == 0)
                    {
                        editor_run();
                    }

                    break;
                }
            }

            if (current_screen == 0)
            {
                const uint32_t now = timer_get_ticks();
                if (now - last_update >= 50)
                {
                    tui_update_dashboard();
                    last_update = now;
                }
            }

            timer_wait(5);
        }
    }
}

