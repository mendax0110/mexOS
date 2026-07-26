#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include "../../shared/types.h"
#include "include/config.h"
#include "arch/i686/idt.h"

#define LET_TIME_PASS(time) \
    for (volatile int k = 0; k < time; k++);
/**
 * @brief Task states
 */
typedef enum
{
    TASK_RUNNING = 0,
    TASK_READY = 1,
    TASK_BLOCKED = 2,
    TASK_ZOMBIE = 3,
} PACKED task_state_t;


/**
 * @brief Macro to define task states and their string representations
 */
#define TASK_STATES              \
    X(TASK_RUNNING, "RUNNING ")  \
    X(TASK_READY,   "READY   ")  \
    X(TASK_BLOCKED, "BLOCKED ")  \
    X(TASK_ZOMBIE,  "ZOMBIE  ")

/**
 * @brief Convert task state to string
 * @param state The task state
 * @return String representation of the task state
 */
const char* task_state_to_string(task_state_t state);

/**
 * @brief Reasons for blocking a task
 */
typedef enum
{
    BLOCK_WAITING = 0,
    BLOCK_SLEEPING = 1,
    BLOCK_IO = 2,
} PACKED block_reason_t;

/**
 * @brief Task priority levels \enum task_priority
 */
enum priority
{
    TASK_PRIORITY_HIGH = 0,
    TASK_PRIORITY_NORMAL = 1,
    TASK_PRIORITY_LOW = 2,
};

/**
 * @brief Macro to define task priorities and their string representations
 */
#define TASK_PRIORITIES                \
    X(TASK_PRIORITY_HIGH,   "HIGH")    \
    X(TASK_PRIORITY_NORMAL, "NORMAL")  \
    X(TASK_PRIORITY_LOW,    "LOW")

/**
 * @brief Convert task priority to string
 * @param priority The task priority
 * @return String representation of the task priority
 */
const char* task_priority_to_string(uint8_t priority);

/**
 * @brief Segment selectors for user mode
 */
#define KERNEL_CS_SEL  0x08
#define KERNEL_DS_SEL  0x10
#define USER_CS_SEL    0x1B
#define USER_DS_SEL    0x23

/**
 * @brief Time in ticks to keep zombie tasks before reaping
 */
#define ZOMBIE_REAP_GRACE_TICKS 200

/**
 * @brief Number of ticks in a CPU usage sampling window
 */
#define CPU_STATS_WINDOW_TICKS 100
#define TASK_CWD_MAX 128
#define TASK_PTY_NONE (-1)
#define TASK_NAME_MAX 16

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
    task_state_t state;
    uint8_t priority;
    uint32_t age;
    uint32_t time_slice;
    bool kernel_mode;
    uint32_t kernel_stack;
    uint32_t kernel_stack_top;
    uint32_t user_stack;
    uint32_t user_stack_top;
    uint32_t user_entry;
    uint32_t cpu_ticks;
    uint32_t heap_next;
    uint32_t window_ticks;
    int32_t exit_code;
    pid_t waiting_for;
    uint32_t wake_at_tick;
    uint32_t exit_tick;
    uint32_t uid;
    uint32_t gid;
    pid_t session_id;
    pid_t process_group;
    char cwd[TASK_CWD_MAX];
    uint32_t cwd_node;
    uint32_t cwd_disk_inode;
    int stdin_pty;
    int stdout_pty;
    char name[TASK_NAME_MAX];
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
pid_t task_fork(struct registers* regs);

/**
 * @brief Wait for a child task to exit
 * @param pid The child PID to wait for, or -1 for any child
 * @param status Pointer to store exit status
 * @return PID of exited child, or -1 on error
 */
pid_t task_wait(pid_t pid, int32_t* status);

/**
 * @brief Wait for a child without necessarily blocking.
 * @param pid Child PID, or -1 for any child
 * @param status Exit status destination
 * @param nohang Return immediately when no child has exited
 */
pid_t task_wait_ex(pid_t pid, int32_t* status, bool nohang);

/**
 * @brief Terminate another task.
 */
int task_kill(pid_t pid, int32_t status);

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
void sched_block(block_reason_t reason);

/**
 * @brief Unblock a task by its ID
 * @param id The task ID to unblock
 */
void sched_unblock(tid_t id);

/**
 * @brief Put the current task to sleep for a specified number of ticks
 * @param ticks The number of ticks to sleep
 */
void sched_sleep(uint32_t ticks);

/**
 * @brief Reap zombie tasks and free their resources
 */
void sched_reap_zombies(void);
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

/**
 * @brief Get the number of ticks elapsed in the current CPU stats
 * @return Ticks since the window last reset
 */
uint32_t sched_get_window_ticks(void);

/**
 * @brief Select the user-space process that adopts orphaned children
 * @param pid The pid to reap
 */
void sched_set_reaper(pid_t pid);


#endif
