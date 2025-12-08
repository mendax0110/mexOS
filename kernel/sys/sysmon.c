#include "sysmon.h"
#include "../ui/console.h"
#include "timer.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../sched/sched.h"

static cpu_stats_t cpu_stats;
static uint32_t last_update_tick = 0;

void sysmon_init(void)
{
    cpu_stats.uptime_ticks = 0;
    cpu_stats.idle_ticks = 0;
    cpu_stats.kernel_ticks = 0;
    cpu_stats.usage_percent = 0;
    last_update_tick = 0;
}

void sysmon_get_memory_stats(memory_stats_t* stats)
{
    if (!stats)
    {
        return;
    }

    stats->total_memory = pmm_get_block_count() * 4096;
    stats->used_memory = pmm_get_used_block_count() * 4096;
    stats->free_memory = pmm_get_free_block_count() * 4096;
    stats->kernel_memory = heap_get_used();
}

void sysmon_get_cpu_stats(cpu_stats_t* stats)
{
    if (!stats)
    {
        return;
    }

    uint32_t total_ticks = sched_get_total_ticks();
    struct task* idle = sched_get_idle_task();

    stats->uptime_ticks = timer_get_ticks();
    
    if (idle)
    {
        stats->idle_ticks = idle->cpu_ticks;
    }
    else
    {
        stats->idle_ticks = 0;
    }

    if (total_ticks > 0)
    {
        stats->usage_percent = ((total_ticks - stats->idle_ticks) * 100) / total_ticks;
    }
    else
    {
        stats->usage_percent = 0;
    }

    stats->kernel_ticks = total_ticks - stats->idle_ticks;
}

void sysmon_get_process_stats(process_stats_t* stats)
{
    if (!stats)
    {
        return;
    }

    stats->total_processes = 0;
    stats->running_processes = 0;
    stats->blocked_processes = 0;
    stats->zombie_processes = 0;

    struct task* t = sched_get_task_list();
    while (t)
    {
        stats->total_processes++;
        
        switch (t->state)
        {
            case TASK_RUNNING:
            case TASK_READY:
                stats->running_processes++;
                break;
            case TASK_BLOCKED:
                stats->blocked_processes++;
                break;
            case TASK_ZOMBIE:
                stats->zombie_processes++;
                break;
            default:
                break;
        }
        
        t = t->next;
    }
}

static void print_memory_size(uint32_t bytes)
{
    if (bytes >= 1024 * 1024)
    {
        console_write_dec(bytes / (1024 * 1024));
        console_write(" MB");
    }
    else if (bytes >= 1024)
    {
        console_write_dec(bytes / 1024);
        console_write(" KB");
    }
    else
    {
        console_write_dec(bytes);
        console_write(" B");
    }
}

void sysmon_print_summary(void)
{
    memory_stats_t mem;
    cpu_stats_t cpu;
    process_stats_t proc;

    sysmon_get_memory_stats(&mem);
    sysmon_get_cpu_stats(&cpu);
    sysmon_get_process_stats(&proc);

    console_write("\n=== System Monitor ===\n\n");
    
    console_write("Memory:\n");
    console_write("  Total:  ");
    print_memory_size(mem.total_memory);
    console_write("\n  Used:   ");
    print_memory_size(mem.used_memory);
    console_write("\n  Free:   ");
    print_memory_size(mem.free_memory);
    console_write("\n  Kernel: ");
    print_memory_size(mem.kernel_memory);
    console_write("\n\n");

    console_write("CPU:\n");
    console_write("  Usage:  ");
    console_write_dec(cpu.usage_percent);
    console_write("%\n");
    console_write("  Uptime: ");
    
    uint32_t seconds = cpu.uptime_ticks / 100;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    console_write_dec(hours);
    console_write("h ");
    console_write_dec(minutes % 60);
    console_write("m ");
    console_write_dec(seconds % 60);
    console_write("s\n\n");

    console_write("Processes:\n");
    console_write("  Total:   ");
    console_write_dec(proc.total_processes);
    console_write("\n  Running: ");
    console_write_dec(proc.running_processes);
    console_write("\n  Blocked: ");
    console_write_dec(proc.blocked_processes);
    console_write("\n  Zombie:  ");
    console_write_dec(proc.zombie_processes);
    console_write("\n");
}

void sysmon_update(void)
{
    uint32_t current_tick = timer_get_ticks();
    
    if (current_tick - last_update_tick >= 100)
    {
        last_update_tick = current_tick;
        cpu_stats.uptime_ticks = current_tick;
    }
}
