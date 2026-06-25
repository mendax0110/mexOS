#include "log.h"
#include "../ui/console.h"
#include "../sched/timer.h"
#include "../lib/string.h"
#include "sync/spinlock.h"

static spinlock_t log_lock = SPINLOCK_INIT;

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

static struct log_entry log_buffer[LOG_MAX_ENTRIES];
static uint32_t log_head = 0;
static uint32_t log_count = 0;
static uint32_t log_sequence = 0;
static uint32_t log_total_written = 0;
static uint32_t log_dropped = 0;

static uint32_t str_len(const char* s)
{
    uint32_t len = 0;
    if (!s) return 0;

    while (*s++)
    {
        len++;
    }

    return len;
}

void log_init(void)
{
    uint32_t flags = spinlock_acquire(&log_lock);

    memset(log_buffer, 0, sizeof(log_buffer));
    log_head = 0;
    log_count = 0;
    log_sequence = 0;
    log_total_written = 0;
    log_dropped = 0;

    spinlock_release(&log_lock, flags);
}

void log_write(const uint8_t level, const char* msg)
{
    if (!msg)
    {
        return;
    }

    uint32_t flags = spinlock_acquire(&log_lock);

    struct log_entry* entry = &log_buffer[log_head];

    entry->sequence = log_sequence++;
    entry->timestamp = timer_get_ticks();
    entry->level = level;
    entry->flags = 0;

    uint32_t len = str_len(msg);
    if (len >= LOG_MAX_MSG_LEN)
    {
        entry->flags |= LOG_FLAG_TRUNCATE;
    }

    strncpy(entry->message, msg, LOG_MAX_MSG_LEN - 1);
    entry->message[LOG_MAX_MSG_LEN - 1] = '\0';

    log_total_written++;

    if (log_count < LOG_MAX_ENTRIES)
    {
        log_count++;
    }
    else
    {
        log_dropped++;
    }

    log_head = (log_head + 1) % LOG_MAX_ENTRIES;

    spinlock_release(&log_lock, flags);
}

void log_debug(const char* msg) { log_write(LOG_LEVEL_DEBUG, msg); }
void log_info(const char* msg)  { log_write(LOG_LEVEL_INFO, msg); }
void log_warn(const char* msg)  { log_write(LOG_LEVEL_WARN, msg); }
void log_error(const char* msg) { log_write(LOG_LEVEL_ERROR, msg); }

uint32_t log_get_count(void)
{
    return log_count;
}

uint32_t log_get_total_written(void)
{
    return log_total_written;
}

uint32_t log_get_dropped(void)
{
    return log_dropped;
}

const struct log_entry* log_get_entry(uint32_t index)
{
    if (index >= log_count)
    {
        return NULL;
    }

    uint32_t start = (log_count >= LOG_MAX_ENTRIES) ? log_head : 0;
    uint32_t actual_idx = (start + index) % LOG_MAX_ENTRIES;

    return &log_buffer[actual_idx];
}

void log_clear(void)
{
    log_init();
}

const char* log_level_to_string(log_level_t level)
{
    switch (level)
    {
#define X(lvl, str) case lvl: return str;
        LOG_LEVELS
#undef X
        default: return "???";
    }
}

void log_stats(void)
{
    uint32_t flags = spinlock_acquire(&log_lock);

    uint32_t count = log_count;
    uint32_t total = log_total_written;
    uint32_t dropped = log_dropped;
    uint32_t sequence = log_sequence;

    spinlock_release(&log_lock, flags);

    console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    console_write("=== Log Buffer Statistics ===\n");
    console_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    console_write("Buffer size:     ");
    console_write_dec(LOG_MAX_ENTRIES);
    console_write(" entries (");
    console_write_dec(sizeof(struct log_entry) * LOG_MAX_ENTRIES);
    console_write(" bytes)\n");

    console_write("Message max:     ");
    console_write_dec(LOG_MAX_MSG_LEN);
    console_write(" bytes\n");

    console_write("Entries stored:  ");
    console_write_dec(count);
    console_write("\n");

    console_write("Total written:   ");
    console_write_dec(total);
    console_write("\n");

    console_write("Overwritten:     ");
    if (dropped > 0)
    {
        console_set_color(VGA_LIGHT_RED, VGA_BLACK);
        console_write_dec(dropped);
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);

        if (total > 0)
        {
            uint32_t pct = (dropped * 100) / total;
            console_write(" (");
            console_write_dec(pct);
            console_write("%)");
        }
    }
    else
    {
        console_write("0");
    }
    console_write("\n");

    console_write("Next sequence:   ");
    console_write_dec(sequence);
    console_write("\n");

    console_write("Buffer usage:    ");
    uint32_t pct_used = (count * 100) / LOG_MAX_ENTRIES;
    console_write_dec(pct_used);
    console_write("%\n");

    console_write("=============================\n");
}

void log_dump(void)
{
    uint32_t flags = spinlock_acquire(&log_lock);

    uint32_t count = log_count;
    uint32_t total = log_total_written;
    uint32_t dropped = log_dropped;
    uint32_t sequence = log_sequence;

    if (count == 0)
    {
        spinlock_release(&log_lock, flags);
        console_write("=== System Log ===\n");
        console_write("Log is empty\n");
        console_write("==================\n");
        return;
    }

    uint32_t start;
    if (count < LOG_MAX_ENTRIES)
    {
        start = 0;
    }
    else
    {
        start = log_head;
    }

    uint32_t indices[LOG_MAX_ENTRIES];
    for (uint32_t i = 0; i < count; i++)
    {
        indices[i] = (start + i) % LOG_MAX_ENTRIES;
    }

    spinlock_release(&log_lock, flags);

    console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    console_write("=== System Log ===\n");

    console_write("Entries: ");
    console_write_dec(count);
    console_write(" shown / ");
    console_write_dec(total);
    console_write(" total");
    if (dropped > 0)
    {
        console_set_color(VGA_LIGHT_RED, VGA_BLACK);
        console_write(" (");
        console_write_dec(dropped);
        console_write(" overwritten)");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
    console_write(" | Seq: ");
    console_write_dec(sequence);
    console_write("\n");
    console_write("----------------------------------------\n");

    for (uint32_t i = 0; i < count; i++)
    {
        const struct log_entry* entry = &log_buffer[indices[i]];

        const uint32_t secs = entry->timestamp / 100;
        const uint32_t ms = (entry->timestamp % 100) * 10;

        console_write("[");
        console_write_dec(secs);
        console_write(".");
        if (ms < 100) console_write("0");
        if (ms < 10) console_write("0");
        console_write_dec(ms);
        console_write("] ");

        switch (entry->level)
        {
            case LOG_LEVEL_DEBUG:
                console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
                break;
            case LOG_LEVEL_INFO:
                console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
                break;
            case LOG_LEVEL_WARN:
                console_set_color(VGA_LIGHT_BROWN, VGA_BLACK);
                break;
            case LOG_LEVEL_ERROR:
                console_set_color(VGA_LIGHT_RED, VGA_BLACK);
                break;
            default:
                console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
                break;
        }

        console_write(log_level_to_string(entry->level));
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write(" ");
        console_write(entry->message);

        if (entry->flags & LOG_FLAG_TRUNCATE)
        {
            console_set_color(VGA_LIGHT_RED, VGA_BLACK);
            console_write(" [TRUNCATED]");
            console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        }

        console_write("\n");
    }

    console_write("==================\n");

    //log_stats();
}

static void format_log_message(char* buffer, size_t buffer_size, const char* format, va_list args)
{
    char* ptr = buffer;
    const char* end = buffer + buffer_size - 1;
    const char* fmt = format;

    while (*fmt && ptr < end)
    {
        if (*fmt == '%' && *(fmt + 1))
        {
            fmt++;

            int width = 0;
            if (*fmt == '0') fmt++;

            while (*fmt >= '0' && *fmt <= '9')
            {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            if (*fmt == 'd' || *fmt == 'u')
            {
                const int val = va_arg(args, int);
                char tmp[16];
                int_to_str_pad(val, tmp, width > 0 ? width : 1);

                for (const char* t = tmp; *t && ptr < end; t++)
                {
                    *ptr++ = *t;
                }
            }
            else if (*fmt == 'x')
            {
                const uint32_t val = va_arg(args, uint32_t);
                char tmp[16];
                int_to_hex_pad(val, tmp, width > 0 ? width : 8);

                for (const char* t = tmp; *t && ptr < end; t++)
                {
                    *ptr++ = *t;
                }
            }
            else if (*fmt == 'p')
            {
                const uint32_t val = va_arg(args, uint32_t);
                char tmp[16];

                if (ptr + 2 < end)
                {
                    *ptr++ = '0';
                    *ptr++ = 'x';
                }

                int_to_hex_pad(val, tmp, 8);

                for (const char* t = tmp; *t && ptr < end; t++)
                {
                    *ptr++ = *t;
                }
            }
            else if (*fmt == 's')
            {
                const char* str = va_arg(args, const char*);
                if (str)
                {
                    while (*str && ptr < end)
                    {
                        *ptr++ = *str++;
                    }
                }
            }
            else
            {
                if (ptr < end)
                {
                    *ptr++ = *fmt;
                }
            }

            fmt++;
        }
        else
        {
            *ptr++ = *fmt++;
        }
    }

    *ptr = '\0';
}

void log_info_fmt(const char* format, ...)
{
    char buffer[LOG_MAX_MSG_LEN];
    va_list args;
    va_start(args, format);
    format_log_message(buffer, LOG_MAX_MSG_LEN, format, args);
    va_end(args);
    log_info(buffer);
}

void log_warn_fmt(const char* format, ...)
{
    char buffer[LOG_MAX_MSG_LEN];
    va_list args;
    va_start(args, format);
    format_log_message(buffer, LOG_MAX_MSG_LEN, format, args);
    va_end(args);
    log_warn(buffer);
}

void log_error_fmt(const char* format, ...)
{
    char buffer[LOG_MAX_MSG_LEN];
    va_list args;
    va_start(args, format);
    format_log_message(buffer, LOG_MAX_MSG_LEN, format, args);
    va_end(args);
    log_error(buffer);
}