#ifndef KERNEL_SYSMON_H
#define KERNEL_SYSMON_H

#include "../include/types.h"

/**
 * @brief Memory statistics structure
 */
typedef struct memory_stats
{
    uint32_t total_memory;
    uint32_t used_memory;
    uint32_t free_memory;
    uint32_t kernel_memory;
} memory_stats_t;

/**
 * @brief CPU statistics structure
 */
typedef struct cpu_stats
{
    uint32_t uptime_ticks;
    uint32_t idle_ticks;
    uint32_t kernel_ticks;
    uint32_t usage_percent;
} cpu_stats_t;

/**
 * @brief Process statistics structure
 */
typedef struct process_stats
{
    uint32_t total_processes;
    uint32_t running_processes;
    uint32_t blocked_processes;
    uint32_t zombie_processes;
} process_stats_t;

/**
 * @brief Initialize system monitoring subsystem
 */
void sysmon_init(void);

/**
 * @brief Get current memory statistics
 * @param stats Pointer to memory_stats_t structure to fill
 */
void sysmon_get_memory_stats(memory_stats_t* stats);

/**
 * @brief Get current CPU statistics
 * @param stats Pointer to cpu_stats_t structure to fill
 */
void sysmon_get_cpu_stats(cpu_stats_t* stats);

/**
 * @brief Get current process statistics
 * @param stats Pointer to process_stats_t structure to fill
 */
void sysmon_get_process_stats(process_stats_t* stats);

/**
 * @brief Print system summary to console
 */
void sysmon_print_summary(void);

/**
 * @brief Update monitoring statistics (called periodically)
 */
void sysmon_update(void);

#endif
