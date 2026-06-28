#include "alloc_track.h"
#include "include/cast.h"
#include "lib/log.h"
#include "ui/console.h"
#include "sync/spinlock.h"
#include "lib/string.h"

/**
 * @brief Allocation record structure \struct alloc_record_t
 */
typedef struct
{
    void* ptr;
    size_t size;
    alloc_src_t src;
    const char* file;
    int line;
    bool in_use;
} alloc_record_t;

static alloc_record_t records[ALLOC_TRACK_MAX];
static uint32_t record_count = 0;
static spinlock_t alloc_lock = SPINLOCK_INIT;
static bool alloc_track_initialized = false;

void alloc_track_add(void* ptr, const size_t size, const alloc_src_t src, const char* file, const int line)
{
    ASSERT(ptr != NULL);
    const uint32_t flags = spinlock_acquire(&alloc_lock);

    TRY_CTX(__FUNCTION__, LAMBDA(void, (void), {
            spinlock_release(&alloc_lock, flags);
    }))
    {
        for (uint32_t i = 0; i < ALLOC_TRACK_MAX; i++)
        {
            if (!records[i].in_use)
            {
                records[i] = (alloc_record_t)
                {
                    .ptr = ptr,
                    .size = size,
                    .src = src,
                    .file = file,
                    .line = line,
                    .in_use = true
                };
                if (i >= record_count)
                {
                    record_count = i + 1;
                }
                spinlock_release(&alloc_lock, flags);
                return;
            }
        }

        spinlock_release(&alloc_lock, flags);
        log_error_fmt("Allocation tracker is full, cannot track allocation at %s:%d", file, line);
    }
}

void alloc_track_remove(void* ptr, const alloc_src_t src, const char* file, const int line)
{
    ASSERT(ptr != NULL);
    const uint32_t flags = spinlock_acquire(&alloc_lock);

    TRY_CTX(__FUNCTION__, LAMBDA(void, (void), {
            spinlock_release(&alloc_lock, flags);
    }))
    {
        for (uint32_t i = 0; i < record_count; i++)
        {
            if (records[i].in_use && records[i].ptr == ptr && records[i].src == src)
            {
                records[i].in_use = false;
                spinlock_release(&alloc_lock, flags);
                return;
            }
        }

        spinlock_release(&alloc_lock, flags);
        log_error_fmt("Allocation at %p not found in tracker for removal (src: %d, location: %s:%d)", ptr, src, file, line);
    }
}

void alloc_track_dump(void)
{
    const uint32_t flags = spinlock_acquire(&alloc_lock);

    TRY_CTX(__FUNCTION__, LAMBDA(void, (void), {
            spinlock_release(&alloc_lock, flags);
    }))
    {
        log_info("Allocation Tracker Dump:");
        console_write("Allocation Tracker Dump:\n");

        for (uint32_t i = 0; i < record_count; i++)
        {
            if (records[i].in_use)
            {
                log_info_fmt("  [%d] ptr: %p, size: %zu, src: %d, location: %s:%d",
                    i, records[i].ptr, records[i].size, records[i].src, records[i].file, records[i].line);
                console_write("  [");
                console_write_dec(i);
                console_write("] ptr: ");
                console_write_hex((uint32_t)records[i].ptr);
                console_write(", size: ");
                console_write_dec(records[i].size);
                console_write(", src: ");
                console_write_dec(records[i].src);
                console_write(", location: ");
                console_write(records[i].file);
                console_write(":");
                console_write_dec(records[i].line);
                console_write("\n");
            }
        }
        spinlock_release(&alloc_lock, flags);
    }
}

uint32_t alloc_track_live_count(void)
{
    uint32_t count = 0;
    const uint32_t flags = spinlock_acquire(&alloc_lock);

    TRY_CTX(__FUNCTION__, LAMBDA(void, (void), {
            spinlock_release(&alloc_lock, flags);
    }))
    {
        for (uint32_t i = 0; i < record_count; i++)
        {
            if (records[i].in_use)
            {
                count++;
            }
        }
        spinlock_release(&alloc_lock, flags);
    }

    return count;
}

uint32_t alloc_track_live_bytes(void)
{
    uint32_t total_size = 0;
    const uint32_t flags = spinlock_acquire(&alloc_lock);

    TRY_CTX(__FUNCTION__, LAMBDA(void, (void), {
            spinlock_release(&alloc_lock, flags);
    }))
    {
        for (uint32_t i = 0; i < record_count; i++)
        {
            if (records[i].in_use)
            {
                total_size += records[i].size;
            }
        }
        spinlock_release(&alloc_lock, flags);
    }

    return total_size;
}

void alloc_track_init(void)
{
    const uint32_t flags = spinlock_acquire(&alloc_lock);
    memset(records, 0, sizeof(records));
    record_count = 0;
    alloc_track_initialized = true;
    spinlock_release(&alloc_lock, flags);
}

bool alloc_track_is_initialized(void)
{
    const uint32_t flags = spinlock_acquire(&alloc_lock);
    const bool initialized = alloc_track_initialized;
    spinlock_release(&alloc_lock, flags);
    return initialized;
}