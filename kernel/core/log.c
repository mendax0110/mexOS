#include "log.h"
#include "console.h"
#include "timer.h"
#include "../include/string.h"

static struct log_entry log_buffer[LOG_MAX_ENTRIES];
static uint32_t log_head = 0;
static uint32_t log_count = 0;

void log_init(void)
{
    memset(log_buffer, 0, sizeof(log_buffer));
    log_head = 0;
    log_count = 0;
}

void log_write(uint8_t level, const char* msg)
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

static const char* level_str(uint8_t level)
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

        uint32_t secs = entry->timestamp / 100;
        uint32_t ms = (entry->timestamp % 100) * 10;

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
