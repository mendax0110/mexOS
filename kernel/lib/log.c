#include "log.h"

#include "fs/fs.h"
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

    const uint32_t len = strlen(msg);
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

int log_save(const char* path)
{
    if (!path || path[0] == '\0')
    {
        return FS_ERR_INVALID;
    }

    const uint32_t flags = spinlock_acquire(&log_lock);

    const uint32_t count = log_count;
    const uint32_t total = log_total_written;
    const uint32_t dropped = log_dropped;
    const uint32_t sequence = log_sequence;
    const uint32_t start = (count >= LOG_MAX_ENTRIES) ? log_head : 0;

    uint32_t indices[LOG_MAX_ENTRIES];
    for (uint32_t i = 0; i < count; i++)
    {
        indices[i] = (start + i) % LOG_MAX_ENTRIES;
    }

    spinlock_release(&log_lock, flags);

    if (!fs_exists("/var"))
    {
        fs_create_dir("/var");
    }
    if (!fs_exists("/var/log"))
    {
        fs_create_dir("/var/log");
    }

    if (!fs_exists(path))
    {
        const int create_ret = fs_create_file(path);
        if (create_ret != FS_ERR_OK)
        {
            return create_ret;
        }
    }

    const int fd = fs_open(path, FS_OPEN_WRITE);
    if (fd < 0)
    {
        return fd;
    }

    t_log_storage header;
    header.magic = LOG_STORAGE_MAGIC;
    header.version = LOG_STORAGE_VERSION;
    header.count = count;
    header.sequence = sequence;
    header.total_written = total;
    header.dropped = dropped;

    int ret = fs_write_fd(fd, (const char*)&header, sizeof(header));
    if (ret != (int)sizeof(header))
    {
        fs_close(fd);
        return FS_ERR_INVALID;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        ret = fs_write_fd(fd, (const char*)&log_buffer[indices[i]], sizeof(struct log_entry));
        if (ret != (int)sizeof(struct log_entry))
        {
            fs_close(fd);
            return FS_ERR_INVALID;
        }
    }

    fs_close(fd);
    fs_sync();

    return FS_ERR_OK;
}

int log_load(const char* path)
{
    if (!path || path[0] == '\0')
    {
        log_warn("log_load: invalid path");
        return FS_ERR_INVALID;
    }

    if (!fs_exists(path))
    {
        log_warn("log_load: log file does not exist");
        return FS_ERR_NOT_FOUND;
    }

    const int fd = fs_open(path, FS_OPEN_READ);
    if (fd < 0)
    {
        log_warn("log_load: failed to open log file, starting with empty log");
        return fd;
    }

    t_log_storage header;
    int ret = fs_read_fd(fd, (char*)&header, sizeof(header));
    if (ret != (int)sizeof(header) ||
        header.magic != LOG_STORAGE_MAGIC ||
        header.version != LOG_STORAGE_VERSION)
    {
        log_warn_fmt("log_load: invalid log file format (magic: 0x%X, version: %d)", header.magic, header.version);
        fs_close(fd);
        return FS_ERR_INVALID;
    }

    uint32_t count = header.count;
    if (count > LOG_MAX_ENTRIES)
    {
        count = LOG_MAX_ENTRIES;
    }

    struct log_entry entry;
    uint32_t loaded = 0;

    const uint32_t flags = spinlock_acquire(&log_lock);
    memset(log_buffer, 0, sizeof(log_buffer));
    spinlock_release(&log_lock, flags);

    for (; loaded < count; loaded++)
    {
        ret = fs_read_fd(fd, (char*)&entry, sizeof(entry));
        if (ret != (int)sizeof(entry))
        {
            break;
        }

        const uint32_t lflags = spinlock_acquire(&log_lock);
        log_buffer[loaded] = entry;
        spinlock_release(&log_lock, lflags);
    }

    fs_close(fd);

    const uint32_t final_flags = spinlock_acquire(&log_lock);
    log_head = loaded % LOG_MAX_ENTRIES;
    log_count = loaded;
    log_sequence = header.sequence;
    log_total_written = header.total_written;
    log_dropped = header.dropped;
    spinlock_release(&log_lock, final_flags);

    return FS_ERR_OK;
}