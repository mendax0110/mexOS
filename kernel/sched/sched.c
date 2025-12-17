#include "sched.h"
#include "../mm/heap.h"
#include "../../shared/string.h"
#include "../arch/i686/gdt.h"
#include "../arch/i686/arch.h"
#include "../include/cast.h"
#include "core/elf.h"
#include "../../shared/log.h"

static struct task* task_queue = NULL;
static struct task* current_task = NULL;
static tid_t next_tid = 1;
static uint32_t tick_count = 0;


void sched_init(void)
{
    task_queue = NULL;
    current_task = NULL;
    next_tid = 1;
    tick_count = 0;
    log_info_fmt("sched: Scheduler with: task struct size %d bytes initialized", sizeof(struct task));
}

struct task* sched_get_task_list(void)
{
    return task_queue;
}

struct task* task_create(void (*entry)(void), const uint8_t priority, const bool kernel_mode)
{
    struct task* t = (struct task*)kmalloc(sizeof(struct task));
    if (!t)
    {
        log_error_fmt("sched: task_create: Failed to allocate memory for new task");
        return NULL;
    }

    memset(t, 0, sizeof(struct task));
    t->id = next_tid++;
    t->pid = (pid_t)t->id;
    t->parent_pid = current_task ? current_task->pid : 0;
    t->state = TASK_READY;
    t->priority = priority;
    t->time_slice = 10;
    t->kernel_mode = kernel_mode;
    t->exit_code = 0;
    t->waiting_for = 0;

    t->kernel_stack = PTR_TO_U32(kmalloc(KERNEL_STACK_SIZE));
    if (!t->kernel_stack)
    {
        kfree(t);
        log_error_fmt("sched: task_create: Failed to allocate memory for kernel stack");
        return NULL;
    }
    t->kernel_stack_top = t->kernel_stack + KERNEL_STACK_SIZE;

    uint32_t* kstack = (uint32_t*)PTR_FROM_U32(t->kernel_stack_top);

    if (kernel_mode)
    {
        kstack[-1] = FUNC_PTR_TO_U32(entry);
        kstack[-2] = 0;
        kstack[-3] = 0;
        kstack[-4] = 0;
        kstack[-5] = 0;

        t->context.esp = PTR_TO_U32(&kstack[-5]);
        t->context.eip = FUNC_PTR_TO_U32(entry);
        t->context.eflags = 0x202;
        log_info_fmt("sched: task_create: Created kernel-mode task (TID %d)", t->id);
    }
    else
    {
        kfree(PTR_FROM_U32(t->kernel_stack));
        kfree(t);
        log_error_fmt("sched: task_create: User-mode task creation not supported in task_create");
        return NULL;
    }

    t->next = task_queue;
    task_queue = t;

    return t;
}

struct task* task_create_user(uint32_t entry_point, const uint8_t priority, page_directory_t* pd)
{
    struct task* t = (struct task*)kmalloc(sizeof(struct task));
    if (!t) return NULL;

    memset(t, 0, sizeof(struct task));
    t->id = next_tid++;
    t->pid = (pid_t)t->id;
    t->parent_pid = current_task ? current_task->pid : 0;
    t->state = TASK_READY;
    t->priority = priority;
    t->time_slice = 10;
    t->kernel_mode = false;
    t->exit_code = 0;

    // Kernel stack
    t->kernel_stack = PTR_TO_U32(kmalloc(KERNEL_STACK_SIZE));
    if (!t->kernel_stack) { kfree(t); return NULL; }
    t->kernel_stack_top = t->kernel_stack + KERNEL_STACK_SIZE;

    // User stack
    t->user_stack = PTR_TO_U32(kmalloc(USER_STACK_SIZE));
    if (!t->user_stack)
    {
        kfree(PTR_FROM_U32(t->kernel_stack));
        kfree(t);
        return NULL;
    }
    t->user_stack_top = t->user_stack + USER_STACK_SIZE;

    t->page_directory = pd;

    // Build iret frame on kernel stack
    uint32_t* kstack = (uint32_t*)PTR_FROM_U32(t->kernel_stack_top);
    *(--kstack) = USER_DS;           // SS
    *(--kstack) = t->user_stack_top; // ESP
    *(--kstack) = 0x202;             // EFLAGS
    *(--kstack) = USER_CS;           // CS
    *(--kstack) = entry_point;       // EIP

    t->context.esp = PTR_TO_U32(kstack);
    t->context.eip = (uint32_t)user_task_trampoline;  // simple iret trampoline
    t->context.eflags = 0x200;
    t->context.cr3 = PTR_TO_U32(pd);
    t->context.kernel_mode = 0;

    // Add to task queue
    t->next = task_queue;
    task_queue = t;

    log_info_fmt("sched: task_create_user: Created user-mode task (TID %d, entry 0x%X)", t->id, entry_point);
    return t;
}

void task_destroy(const tid_t id)
{
    struct task* prev = NULL;
    struct task* t = task_queue;

    while (t)
    {
        if (t->id == id)
        {
            if (prev) prev->next = t->next;
            else task_queue = t->next;

            if (t->kernel_stack) kfree(PTR_FROM_U32(t->kernel_stack));
            if (t->user_stack) kfree(PTR_FROM_U32(t->user_stack));
            kfree(t);
            log_info_fmt("sched: task_destroy: Destroyed task (TID %d)", id);
            return;
        }
        prev = t;
        t = t->next;
    }
}

void task_exit(const tid_t id, const int32_t exit_code)
{
    struct task* t = task_queue;
    while (t)
    {
        if (t->id == id)
        {
            t->state = TASK_ZOMBIE;
            t->exit_code = exit_code;

            struct task* parent = task_find(t->parent_pid);
            if (parent && parent->state == TASK_BLOCKED &&
                (parent->waiting_for == t->pid || parent->waiting_for == -1))
            {
                parent->state = TASK_READY;
                log_info_fmt("sched: task_exit: Unblocked parent task (TID %d) waiting for child (TID %d)", parent->id, t->id);
            }
            return;
        }
        t = t->next;
    }
}

struct task* task_find(const pid_t pid)
{
    struct task* t = task_queue;
    while (t)
    {
        if (t->pid == pid)
        {
            log_info_fmt("sched: task_find: Found task (TID %d) for PID %d", t->id, pid);
            return t;
        }
        t = t->next;
    }
    return NULL;
}

pid_t task_fork(void)
{
    if (!current_task)
    {
        log_warn_fmt("sched: task_fork: No current task to fork");
        return -1;
    }

    struct task* child = (struct task*)kmalloc(sizeof(struct task));
    if (!child)
    {
        log_warn_fmt("sched: task_fork: Failed to allocate memory for child task");
        return -1;
    }

    memcpy(child, current_task, sizeof(struct task));
    child->id = next_tid++;
    child->pid = (pid_t)child->id;
    child->parent_pid = current_task->pid;
    child->state = TASK_READY;
    child->time_slice = 10;
    child->cpu_ticks = 0;
    child->exit_code = 0;
    child->waiting_for = 0;

    child->kernel_stack = PTR_TO_U32(kmalloc(KERNEL_STACK_SIZE));
    if (!child->kernel_stack)
    {
        kfree(child);
        log_error_fmt("sched: task_fork: Failed to allocate memory for child kernel stack");
        return -1;
    }
    child->kernel_stack_top = child->kernel_stack + KERNEL_STACK_SIZE;

    memcpy(PTR_FROM_U32(child->kernel_stack), PTR_FROM_U32(current_task->kernel_stack), KERNEL_STACK_SIZE);

    uint32_t stack_offset = current_task->context.esp - current_task->kernel_stack;
    child->context.esp = child->kernel_stack + stack_offset;

    if (!current_task->kernel_mode && current_task->user_stack)
    {
        child->user_stack = PTR_TO_U32(kmalloc(USER_STACK_SIZE));
        if (!child->user_stack)
        {
            kfree(PTR_FROM_U32(child->kernel_stack));
            kfree(child);
            log_error_fmt("sched: task_fork: Failed to allocate memory for child user stack");
            return -1;
        }
        child->user_stack_top = child->user_stack + USER_STACK_SIZE;
        memcpy(PTR_FROM_U32(child->user_stack), PTR_FROM_U32(current_task->user_stack), USER_STACK_SIZE);
        log_info_fmt("sched: task_fork: Copied user stack for child task (TID %d)", child->id);
    }

    child->context.eax = 0;

    child->next = task_queue;
    task_queue = child;

    return child->pid;
}

pid_t task_wait(const pid_t pid, int32_t* status)
{
    if (!current_task)
    {
        log_warn_fmt("sched: task_wait: No current task to wait");
        return -1;
    }

    while (1)
    {
        struct task* t = task_queue;
        while (t)
        {
            if (t->parent_pid == current_task->pid)
            {
                if ((pid == -1 || t->pid == pid) && t->state == TASK_ZOMBIE)
                {
                    pid_t child_pid = t->pid;
                    if (status)
                    {
                        log_info_fmt("sched: task_wait: Retrieved exit status %d for child task (TID %d)", t->exit_code, t->id);
                        *status = t->exit_code;
                    }
                    task_destroy(t->id);
                    return child_pid;
                }
            }
            t = t->next;
        }

        bool has_children = false;
        t = task_queue;
        while (t)
        {
            if (t->parent_pid == current_task->pid)
            {
                if (pid == -1 || t->pid == pid)
                {
                    log_info_fmt("sched: task_wait: Current task (TID %d) has children to wait for", current_task->id);
                    has_children = true;
                    break;
                }
            }
            t = t->next;
        }

        if (!has_children)
        {
            log_info_fmt("sched: task_wait: Current task (TID %d) has no children to wait for", current_task->id);
            return -1;
        }

        current_task->waiting_for = pid;
        current_task->state = TASK_BLOCKED;
        schedule();
    }
}

static struct task* pick_next_task(void)
{
    struct task* best = NULL;
    struct task* t = task_queue;

    while (t)
    {
        if (t->state == TASK_READY)
        {
            if (!best || t->priority > best->priority)
            {
                log_info_fmt("sched: pick_next_task: Considering task (TID %d) with priority %d", t->id, t->priority);
                best = t;
            }
        }
        t = t->next;
    }
    return best;
}

void schedule(void)
{
    if (!task_queue) return;

    struct task* next = pick_next_task();
    if (!next)
    {
        log_warn_fmt("sched: schedule: No READY tasks to schedule");
        return;
    }

    if (current_task && current_task->state == TASK_RUNNING)
    {
        log_info_fmt("sched: schedule: Setting current task (TID %d) state to READY", current_task->id);
        current_task->state = TASK_READY;
    }

    struct task* old = current_task;
    current_task = next;
    current_task->state = TASK_RUNNING;
    current_task->time_slice = 10;

    if (current_task->kernel_stack)
    {
        log_info_fmt("sched: schedule: Setting TSS kernel stack for task (TID %d)", current_task->id);
        tss_set_kernel_stack(current_task->kernel_stack + KERNEL_STACK_SIZE);
    }

    if (!current_task->kernel_mode && current_task->page_directory)
    {
        log_info_fmt("sched: schedule: Switching address space for user-mode task (TID %d)", current_task->id);
        vmm_switch_address_space(current_task->page_directory);
    }

    if (old && old != current_task)
    {
        log_info_fmt("sched: schedule: Switching context from task (TID %d) to task (TID %d)", old->id, current_task->id);
        switch_context(&old->context, &current_task->context);
    }
    else if (!old)
    {
        log_info_fmt("sched: schedule: Switching to first task (TID %d)", current_task->id);
        switch_context(NULL, &current_task->context);
    }
}


void sched_yield(void)
{
    schedule();
}

void sched_tick(void)
{
    tick_count++;

    if (current_task)
    {
        current_task->cpu_ticks++;

        if (current_task->time_slice > 0)
        {
            current_task->time_slice--;
        }

        if (current_task->time_slice == 0)
        {
            schedule();
        }
    }
}

struct task* sched_get_current(void)
{
    return current_task;
}

void sched_block(const uint8_t reason)
{
    (void)reason;
    if (current_task)
    {
        current_task->state = TASK_BLOCKED;
        log_info_fmt("sched: sched_block: Blocking current task (TID %d) for reason %d", current_task->id, reason);
        schedule();
    }
}

void sched_unblock(const tid_t id)
{
    struct task* t = task_queue;
    while (t)
    {
        if (t->id == id)
        {
            t->state = TASK_READY;
            log_info_fmt("sched: sched_unblock: Unblocked task (TID %d)", id);
            return;
        }
        t = t->next;
    }
}

uint32_t sched_get_total_ticks(void)
{
    return tick_count;
}

struct task* sched_get_idle_task(void)
{
    struct task* t = task_queue;
    while (t)
    {
        if (t->priority == 0)
        {
            return t;
        }
        t = t->next;
    }
    return NULL;
}
