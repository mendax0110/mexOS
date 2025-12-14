#include "log.h"
#include "../../servers/console/console.h"
#include "../sys/timer.h"
#include "../include/string.h"

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

static struct log_entry log_buffer[LOG_MAX_ENTRIES];
static uint32_t log_head = 0;
static uint32_t log_count = 0;

void log_init(void)
{
    memset(log_buffer, 0, sizeof(log_buffer));
    log_head = 0;
    log_count = 0;
}

void log_write(const uint8_t level, const char* msg)
{
    if (msg == NULL)
    {
        return;
    }

    struct log_entry* entry = &log_buffer[log_head];

    entry->timestamp = timer_get_ticks();
    entry->level = level;

    strncpy(entry->message, msg, LOG_MAX_MSG_LEN - 1);
    entry->message[LOG_MAX_MSG_LEN - 1] = '\0';

    log_head = (log_head + 1) % LOG_MAX_ENTRIES;
    if (log_count < LOG_MAX_ENTRIES)
    {
        log_count++;
    }
}

void log_debug(const char* msg)
{
    log_write(LOG_LEVEL_DEBUG, msg);
}

void log_info(const char* msg)
{
    log_write(LOG_LEVEL_INFO, msg);
}

void log_warn(const char* msg)
{
    log_write(LOG_LEVEL_WARN, msg);
}

void log_error(const char* msg)
{
    log_write(LOG_LEVEL_ERROR, msg);
}

uint32_t log_get_count(void)
{
    return log_count;
}

const struct log_entry* log_get_entry(uint32_t index)
{
    if (index >= log_count)
    {
        return NULL;
    }

    uint32_t actual_idx;
    if (log_count < LOG_MAX_ENTRIES)
    {
        actual_idx = index;
    }
    else
    {
        actual_idx = (log_head + index) % LOG_MAX_ENTRIES;
    }

    return &log_buffer[actual_idx];
}

void log_clear(void)
{
    memset(log_buffer, 0, sizeof(log_buffer));
    log_head = 0;
    log_count = 0;
}

static const char* level_str(const uint8_t level)
{
    switch (level)
    {
        case LOG_LEVEL_DEBUG: return "DBG";
        case LOG_LEVEL_INFO:  return "INF";
        case LOG_LEVEL_WARN:  return "WRN";
        case LOG_LEVEL_ERROR: return "ERR";
        default:              return "???";
    }
}

void log_dump(void)
{
    if (log_count == 0)
    {
        console_write("Log is empty\n");
        return;
    }

    console_write("=== System Log ===\n");

    for (uint32_t i = 0; i < log_count; i++)
    {
        const struct log_entry* entry = log_get_entry(i);
        if (entry == NULL)
        {
            continue;
        }

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
                console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
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
        }

        console_write(level_str(entry->level));
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write(" ");
        console_write(entry->message);
        console_write("\n");
    }

    console_write("==================\n");
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
            bool zero_pad = false;
            if (*fmt == '0')
            {
                zero_pad = true;
                fmt++;
            }
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
                if (ptr < end) *ptr++ = *fmt;
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