#include "sched.h"
#include "../mm/heap.h"
#include "../mm/vmm.h"
#include "../include/string.h"
#include "../arch/i686/gdt.h"
#include "../include/cast.h"

static struct task* task_queue = NULL;
static struct task* current_task = NULL;
static tid_t next_tid = 1;
static uint32_t tick_count = 0;

static void user_task_entry(void);

void sched_init(void)
{
    task_queue = NULL;
    current_task = NULL;
    next_tid = 1;
    tick_count = 0;
}

struct task* sched_get_task_list(void)
{
    return task_queue;
}

static void user_task_entry(void)
{
    struct task* t = current_task;
    if (!t || t->kernel_mode)
    {
        return;
    }

    enter_usermode(
            t->context.eip,
            t->user_stack_top,
            USER_CS_SEL,
            USER_DS_SEL
    );
}

static struct task* task_alloc(const uint32_t entry_point, const uint8_t priority, const bool kernel_mode)
{
    struct task* t = (struct task*)kmalloc(sizeof(struct task));
    if (!t)
    {
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

    t->kernel_stack = PTR_TO_U32(kmalloc(KERNEL_STACK_SIZE));
    if (!t->kernel_stack)
    {
        kfree(t);
        return NULL;
    }
    t->kernel_stack_top = t->kernel_stack + KERNEL_STACK_SIZE;

    if (!kernel_mode)
    {
        t->user_stack = PTR_TO_U32(kmalloc(USER_STACK_SIZE));
        if (!t->user_stack)
        {
            kfree(PTR_FROM_U32(t->kernel_stack));
            kfree(t);
            return NULL;
        }
        t->user_stack_top = t->user_stack + USER_STACK_SIZE;
    }

    uint32_t* kstack = (uint32_t*)PTR_FROM_U32(t->kernel_stack_top);
    kstack[-1] = kernel_mode ? entry_point : FUNC_PTR_TO_U32(user_task_entry);
    kstack[-2] = 0;
    kstack[-3] = 0;
    kstack[-4] = 0;
    kstack[-5] = 0;

    t->context.esp    = PTR_TO_U32(&kstack[-5]);
    t->context.eip    = entry_point;
    t->context.eflags = 0x202;

    t->next = task_queue;
    task_queue = t;

    return t;
}

struct task* task_create(void (*entry)(void), const uint8_t priority, const bool kernel_mode)
{
    return task_alloc(FUNC_PTR_TO_U32(entry), priority, kernel_mode);
}

struct task* task_create_user(const uint32_t entry_point, const uint8_t priority)
{
    return task_alloc(entry_point, priority, false);
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

            /* Wake a parent that is waiting for this child */
            struct task* parent = task_find(t->parent_pid);
            if (parent && parent->state == TASK_BLOCKED &&
                (parent->waiting_for == t->pid || parent->waiting_for == (pid_t)-1))
            {
                parent->state = TASK_READY;
            }

            /* Reparent orphaned children to init (PID 1) so they get reaped */
            struct task* child = task_queue;
            while (child)
            {
                if (child->parent_pid == t->pid)
                {
                    child->parent_pid = 1;
                }
                child = child->next;
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
        return -1;
    }

    struct task* child = (struct task*)kmalloc(sizeof(struct task));
    if (!child)
    {
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
            return -1;
        }
        child->user_stack_top = child->user_stack + USER_STACK_SIZE;
        memcpy(PTR_FROM_U32(child->user_stack), PTR_FROM_U32(current_task->user_stack), USER_STACK_SIZE);
    }

    child->context.eax = 0;

    if (!current_task->kernel_mode && current_task->context.cr3)
    {
        page_directory_t* parent_pd = (page_directory_t*)(uintptr_t)current_task->context.cr3;
        page_directory_t* child_pd  = vmm_clone_address_space(parent_pd);
        if (!child_pd)
        {
            if (child->user_stack) kfree(PTR_FROM_U32(child->user_stack));
            kfree(PTR_FROM_U32(child->kernel_stack));
            kfree(child);
            return -1;
        }
        child->context.cr3 = PTR_TO_U32(child_pd);
    }

    child->next = task_queue;
    task_queue = child;

    return child->pid;
}

pid_t task_wait(const pid_t pid, int32_t* status)
{
    if (!current_task)
    {
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
                    has_children = true;
                    break;
                }
            }
            t = t->next;
        }

        if (!has_children)
        {
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
    uint32_t best_eff = 0;
    struct task* t = task_queue;

    while (t)
    {
        if (t->state == TASK_READY)
        {
            /* Effective priority = base + age/4 (capped to avoid overflow) */
            const uint32_t eff = (uint32_t)t->priority + (t->age >> 2);
            if (!best || eff > best_eff)
            {
                best = t;
                best_eff = eff;
            }
        }
        t = t->next;
    }
    return best;
}

void schedule(void)
{
    if (!task_queue) return;

    /* Prevent re-entrant calls (e.g. timer ISR firing during schedule), switch_context saves the live EFLAGS
     * into the old task's context, so any cli/sti before the switch would
     * corrupt the saved IF bit and freeze the task on next restore. */
    static volatile uint32_t in_schedule = 0;
    if (in_schedule) return;
    in_schedule = 1;

    struct task* next = pick_next_task();
    if (!next)
    {
        in_schedule = 0;
        return;
    }

    if (current_task && current_task->state == TASK_RUNNING)
    {
        current_task->state = TASK_READY;
    }

    struct task* old = current_task;
    current_task = next;
    current_task->state = TASK_RUNNING;
    current_task->time_slice = 10;
    current_task->age = 0;  /* reset aging when task gets the CPU */

    if (current_task->kernel_stack)
    {
        tss_set_kernel_stack(current_task->kernel_stack + KERNEL_STACK_SIZE);
    }

    in_schedule = 0;

    if (old && old != current_task)
    {
        switch_context(&old->context, &current_task->context);
    }
    else if (!old)
    {
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

    /* Age all ready tasks so they eventually get CPU time */
    struct task* t = task_queue;
    while (t)
    {
        if (t->state == TASK_READY && t->age < 0xFFFFFFFFU)
        {
            t->age++;
        }
        t = t->next;
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
