#ifndef SHARED_SYSTEM_ABI_H
#define SHARED_SYSTEM_ABI_H

#include "types.h"

#define FS_OP_MKDIR 1
#define FS_OP_TOUCH 2
#define FS_OP_REMOVE 3
#define FS_OP_SYNC 4

/**
 * @brief System information structure for userland queries. \struct system_info
 */
struct system_info
{
    uint32_t total_memory_kb;
    uint32_t used_memory_kb;
    uint32_t free_memory_kb;
    uint32_t uptime_ticks;
};

#endif
