#ifndef SHARED_PROCESS_ABI_H
#define SHARED_PROCESS_ABI_H

#include "types.h"

#define WAIT_NOHANG 0x01
#define PROCESS_NAME_MAX 16

/**
 * @brief Struct to represent the process info. \struct process_info
 */
struct process_info
{
    pid_t pid;
    pid_t parent_pid;
    pid_t session_id;
    uint32_t uid;
    uint32_t state;
    uint32_t cpu_ticks;
    char name[PROCESS_NAME_MAX];
};

#define POWER_SHUTDOWN 0
#define POWER_REBOOT 1

#endif
