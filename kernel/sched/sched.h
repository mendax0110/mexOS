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

/**
 * @brief Segment selectors for user mode
 */
#define KERNEL_CS_SEL  0x08
#define KERNEL_DS_SEL  0x10
#define USER_CS_SEL    0x1B
#define USER_DS_SEL    0x23

/**
 * @brief Task context structure for context switching
 * @details Layout matches the stack frame pushed by switch_context
 */
struct task_context
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cr3;
};

/**
 * @brief Interrupt stack frame pushed by CPU on interrupt/exception
 * @details Used for iret to user mode
 */
struct iret_frame
{
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
};

/**
 * @brief Task structure
 */
struct task
{
    tid_t id;
    pid_t pid;
    pid_t parent_pid;
    uint8_t state;
    uint8_t priority;
    uint32_t time_slice;
    bool kernel_mode;
    uint32_t kernel_stack;
    uint32_t kernel_stack_top;
    uint32_t user_stack;
    uint32_t user_stack_top;
    uint32_t cpu_ticks;
    int32_t exit_code;
    pid_t waiting_for;
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
 * @brief Create a new user-mode task from an ELF entry point
 * @param entry_point The user-space entry point address (from ELF)
 * @param priority Task priority
 * @return Pointer to the created task, or NULL on failure
 */
struct task* task_create_user(uint32_t entry_point, uint8_t priority);

/**
 * @brief Destroy a task
 * @param id The task ID to destroy
 */
void task_destroy(tid_t id);

/**
 * @brief Exit a task and set it to zombie state
 * @param id The task ID to exit
 * @param exit_code The exit code
 */
void task_exit(tid_t id, int32_t exit_code);

/**
 * @brief Fork the current task
 * @return Child PID in parent, 0 in child, -1 on error
 */
pid_t task_fork(void);

/**
 * @brief Wait for a child task to exit
 * @param pid The child PID to wait for, or -1 for any child
 * @param status Pointer to store exit status
 * @return PID of exited child, or -1 on error
 */
pid_t task_wait(pid_t pid, int32_t* status);

/**
 * @brief Find a task by PID
 * @param pid The PID to search for
 * @return Pointer to the task, or NULL if not found
 */
struct task* task_find(pid_t pid);

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
 * @brief Enter user mode
 * @param entry User-space entry point (EIP)
 * @param user_stack User-space stack pointer (ESP)
 * @param cs User code segment selector
 * @param ds User data segment selector
 */
extern void enter_usermode(uint32_t entry, uint32_t user_stack, uint32_t cs, uint32_t ds);

/**
 * @brief Get the list of all tasks
 * @return Pointer to the head of the task list
 */
struct task* sched_get_task_list(void);

/**
 * @brief Get the idle task
 * @return Pointer to the idle task
 */
struct task* sched_get_idle_task(void);

/**
 * @brief Get the total number of CPU ticks since boot
 * @return Total CPU ticks
 */
uint32_t sched_get_total_ticks(void);

#ifdef __cplusplus
}
#endif

#endif
