#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include "../include/types.h"
#include "../include/config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Task states
 */
#define TASK_RUNNING   0
#define TASK_READY     1
#define TASK_BLOCKED   2
#define TASK_ZOMBIE    3

/// @brief Task context structure \struct task_context
struct task_context
{
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, eflags;
    uint32_t cr3;
};

/// @brief Task structure \struct task
struct task
{
    tid_t id;
    pid_t pid;
    uint8_t state;
    uint8_t priority;
    uint32_t time_slice;
    bool kernel_mode;
    uint32_t kernel_stack;
    uint32_t user_stack;
    struct task_context context;
    struct task* next;
};

/**
 * @brief Initialize the scheduler
 */
void sched_init(void);

/**
 * @brief Create a new task
 * @param entry Pointer to the task entry function
 * @param priority Task priority
 * @param kernel_mode True if the task runs in kernel mode, false for user mode
 * @return Pointer to the created task, or NULL on failure
 */
struct task* task_create(void (*entry)(void), uint8_t priority, bool kernel_mode);

/**
 * @brief Destroy a task
 * @param id The task ID to destroy
 */
void task_destroy(tid_t id);

/**
 * @brief Schedule the next task to run
 */
void schedule(void);

/**
 * @brief Yield the CPU to allow other tasks to run
 */
void sched_yield(void);

/**
 * @brief Handle a scheduler tick
 */
void sched_tick(void);

/**
 * @brief Get the currently running task
 * @return Pointer to the current task
 */
struct task* sched_get_current(void);

/**
 * @brief Block the current task for a specified reason
 * @param reason The reason code for blocking
 */
void sched_block(uint8_t reason);

/**
 * @brief Unblock a task by its ID
 * @param id The task ID to unblock
 */
void sched_unblock(tid_t id);

/**
 * @brief Switch context between two tasks
 * @param old Pointer to the old task context
 * @param new_ctx Pointer to the new task context
 */
extern void switch_context(struct task_context* old, struct task_context* new_ctx);

/**
 * @brief Get the list of all tasks
 * @return Pointer to the head of the task list
 */
struct task* sched_get_task_list(void);

#ifdef __cplusplus
}
#endif

#endif
