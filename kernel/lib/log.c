#include "log.h"
#include "ui/console.h"
#include "sched/timer.h"
#include "sync/spinlock.h"

static spinlock_t log_lock = SPINLOCK_INIT;

static struct log_entry log_buffer[LOG_MAX_ENTRIES];
static uint32_t log_head = 0;
static uint32_t log_count = 0;
static uint32_t log_sequence = 0;
static uint32_t log_total_written = 0;
static uint32_t log_dropped = 0;

static uint32_t str_len(const char* s)
{
    uint32_t len = 0;
    if (!s) { return 0; }

    while (*s++)
    {
        len++;
    }

    return len;
}

void log_init(void)
{
    const uint32_t flags = spinlock_acquire(&log_lock);

    memset(log_buffer, 0, sizeof(log_buffer));
    log_head = 0;
    log_count = 0;
    log_sequence = 0;
    log_total_written = 0;
    log_dropped = 0;

    spinlock_release(&log_lock, flags);
}

void log_write(const uint8_t level, const char* file, const int line , const char* msg)
{
    if (!msg)
    {
        return;
    }

    const uint32_t flags = spinlock_acquire(&log_lock);

    struct log_entry* entry = &log_buffer[log_head];

    entry->sequence = log_sequence++;
    entry->timestamp = timer_get_ticks();
    entry->level = level;
    entry->flags = 0;

    const uint32_t len = str_len(msg);
    if (len >= LOG_MAX_MSG_LEN)
    {
        entry->flags |= LOG_FLAG_TRUNCATE;
    }

    // append file and line information to the message
    char formatted_msg[LOG_MAX_MSG_LEN];
    snprintf(formatted_msg, LOG_MAX_MSG_LEN, "[%s:%d] %s", file, line, msg);
    strncpy(entry->message, formatted_msg, LOG_MAX_MSG_LEN - 1);
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

const struct log_entry* log_get_entry(const uint32_t index)
{
    if (index >= log_count)
    {
        return NULL;
    }

    const uint32_t start = (log_count >= LOG_MAX_ENTRIES) ? log_head : 0;
    const uint32_t actual_idx = (start + index) % LOG_MAX_ENTRIES;

    return &log_buffer[actual_idx];
}

void log_clear(void)
{
    log_init();
}

const char* log_level_to_string(const log_level_t level)
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
    const uint32_t flags = spinlock_acquire(&log_lock);

    const uint32_t count = log_count;
    const uint32_t total = log_total_written;
    const uint32_t dropped = log_dropped;
    const uint32_t sequence = log_sequence;

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
            const uint32_t pct = (dropped * 100) / total;
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
    const uint32_t pct_used = (count * 100) / LOG_MAX_ENTRIES;
    console_write_dec(pct_used);
    console_write("%\n");

    console_write("=============================\n");
}

void log_dump(void)
{
    const uint32_t flags = spinlock_acquire(&log_lock);

    const uint32_t count = log_count;
    const uint32_t total = log_total_written;
    const uint32_t dropped = log_dropped;
    const uint32_t sequence = log_sequence;

    if (count == 0)
    {
        spinlock_release(&log_lock, flags);
        console_write("=== System Log ===\n");
        console_write("Log is empty\n");
        console_write("==================\n");
        return;
    }

    uint32_t start = 0;
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
        if (ms < 100)
        {
            console_write("0");
        }

        if (ms < 10)
        {
            console_write("0");
        }

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
}