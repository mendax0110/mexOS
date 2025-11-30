#include "sched.h"
#include "../mm/heap.h"
#include "../include/string.h"
#include "../arch/i686/gdt.h"
#include "../arch/i686/arch.h"

/**
 * @brief Task states
 */
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
}

struct task* task_create(void (*entry)(void), const uint8_t priority, const bool kernel_mode)
{
    struct task* t = (struct task*)kmalloc(sizeof(struct task));
    if (!t) return NULL;

    memset(t, 0, sizeof(struct task));
    t->id = next_tid++;
    t->pid = t->id;
    t->state = TASK_READY;
    t->priority = priority;
    t->time_slice = 10;

    const uint32_t stack_size = kernel_mode ? KERNEL_STACK_SIZE : USER_STACK_SIZE;
    t->kernel_stack = (uint32_t)kmalloc(stack_size);
    if (!t->kernel_stack)
    {
        kfree(t);
        return NULL;
    }

    uint32_t* stack = (uint32_t*)(t->kernel_stack + stack_size);

    // Set up stack for context switch
    stack[-1] = (uint32_t)entry;  // Return address (EIP)
    stack[-2] = 0;                 // EBP
    stack[-3] = 0;                 // EBX
    stack[-4] = 0;                 // ESI
    stack[-5] = 0;                 // EDI

    t->context.esp = (uint32_t)&stack[-5];
    t->context.eip = (uint32_t)entry;
    t->context.eflags = 0x202;  // IF enabled

    // Add to queue
    t->next = task_queue;
    task_queue = t;

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

            if (t->kernel_stack) kfree((void*)t->kernel_stack);
            kfree(t);
            return;
        }
        prev = t;
        t = t->next;
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
    if (!next) return;

    if (current_task && current_task->state == TASK_RUNNING)
    {
        current_task->state = TASK_READY;
    }

    struct task* old = current_task;
    current_task = next;
    current_task->state = TASK_RUNNING;
    current_task->time_slice = 10;

    if (current_task->kernel_stack)
    {
        tss_set_kernel_stack(current_task->kernel_stack + KERNEL_STACK_SIZE);
    }

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
