#include "diag/ptr_track.h"
#include "include/assert.h"
#include "lib/log.h"
#include "lib/string.h"

static tracked_ptr_entry_t g_tracked_ptrs[MAX_TRACKED_PTRS];
static char ptr_location_buf[128];

void ptr_track_register(void* ptr, const char* name, const char* file, const int line, const bool in_use)
{
    for (int i = 0; i < MAX_TRACKED_PTRS; i++)
    {
        if (!g_tracked_ptrs[i].in_use)
        {
            g_tracked_ptrs[i] = (tracked_ptr_entry_t)
            {
                .ptr = ptr,
                .name = name,
                .file = file,
                .line = line,
                .in_use = in_use,
            };
            return;
        }
    }
    PANIC_FMT("tracked_ptr table full! Cannot register pointer %p (%s) at %s:%d", ptr, name, file, line);
}

bool ptr_track_unregister(void* ptr)
{
    if (!ptr)
    {
        return false;
    }

    for (int i = 0; i < MAX_TRACKED_PTRS; i++)
    {
        if (g_tracked_ptrs[i].in_use && g_tracked_ptrs[i].ptr == ptr)
        {
            g_tracked_ptrs[i].in_use = false;
            return true;
        }
    }

    return false;
}

const tracked_ptr_entry_t* ptr_track_lookup(void* ptr)
{
    if (!ptr)
    {
        return NULL;
    }

    for (int i = 0; i < MAX_TRACKED_PTRS; i++)
    {
        if (g_tracked_ptrs[i].in_use && g_tracked_ptrs[i].ptr == ptr)
        {
            return &g_tracked_ptrs[i];
        }
    }

    return NULL;
}

void ptr_track_dump(void)
{
    for (int i = 0; i < MAX_TRACKED_PTRS; i++)
    {
        if (g_tracked_ptrs[i].in_use)
        {
            log_info_fmt("Tracked pointer: %p, name: %s, file: %s, line: %d",
                g_tracked_ptrs[i].ptr, g_tracked_ptrs[i].name, g_tracked_ptrs[i].file, g_tracked_ptrs[i].line);
        }
    }
}

const char* ptr_track_location(void* ptr)
{
    const tracked_ptr_entry_t* tracked = ptr_track_lookup(ptr);
    if (tracked)
    {
        snprintf(ptr_location_buf, sizeof(ptr_location_buf), "%s:%d in %s",
                 tracked->file, tracked->line, tracked->name);
    }
    else
    {
        snprintf(ptr_location_buf, sizeof(ptr_location_buf), "unknown location");
    }

    return ptr_location_buf;
}
