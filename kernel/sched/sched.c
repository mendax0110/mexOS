#include "sched.h"
#include "mm/heap.h"
#include "mm/vmm.h"
#include "lib/string.h"
#include "arch/i686/gdt.h"
#include "include/addr.h"
#include "core/rollback.h"
#include "lib/log.h"
#include "sync/spinlock.h"
#include "fs/fs.h"
#include "core/pty.h"
#include "core/shm.h"
#include "ipc/ipc.h"

static struct task* task_queue = NULL;
static struct task* current_task = NULL;
static tid_t next_tid = 1;
static uint32_t tick_count = 0;
static spinlock_t sched_lock = SPINLOCK_INIT;
static uint32_t window_start_tick = 0;
static pid_t reaper_pid = 1;

static void user_task_entry(void);

void sched_init(void)
{
    task_queue = NULL;
    current_task = NULL;
    next_tid = 1;
    tick_count = 0;
    window_start_tick = 0;
    reaper_pid = 1;
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

    if (t->context.cr3)
    {
        vmm_switch_address_space(PTR_FROM_U32_TYPED(page_directory_t, t->context.cr3));
    }

    enter_usermode(
            t->user_entry,
            t->user_stack_top,
            USER_CS_SEL,
            USER_DS_SEL
    );
}

static struct task* task_alloc(const uint32_t entry_point, const uint8_t priority, const bool kernel_mode, const task_state_t initial_state)
{
    struct task* t = kmalloc(sizeof(struct task));
    if (!t)
    {
        return NULL;
    }

    memset(t, 0, sizeof(struct task));
    t->id = next_tid++;
    t->pid = (pid_t)t->id;
    t->parent_pid = current_task ? current_task->pid : 0;
    t->state = initial_state;
    t->priority = priority;
    t->time_slice = 10;
    t->kernel_mode = kernel_mode;
    t->context.cr3 = PTR_TO_U32(vmm_get_kernel_directory());
    t->stdin_pty = TASK_PTY_NONE;
    t->stdout_pty = TASK_PTY_NONE;
    strcpy(t->cwd, "/");
    strcpy(t->name, kernel_mode ? "kernel" : "user");
    t->cwd_node = 0;
    t->cwd_disk_inode = 0;
    if (current_task)
    {
        t->uid = current_task->uid;
        t->gid = current_task->gid;
        t->session_id = current_task->session_id;
        t->process_group = current_task->process_group;
        strncpy(t->cwd, current_task->cwd, sizeof(t->cwd) - 1);
        t->cwd[sizeof(t->cwd) - 1] = '\0';
        t->cwd_node = current_task->cwd_node;
        t->cwd_disk_inode = current_task->cwd_disk_inode;
        t->stdin_pty = current_task->stdin_pty;
        t->stdout_pty = current_task->stdout_pty;
    }
    else
    {
        t->session_id = t->pid;
        t->process_group = t->pid;
    }

    t->kernel_stack = PTR_TO_U32(kmalloc(KERNEL_STACK_SIZE));
    if (!t->kernel_stack)
    {
        kfree(t);
        return NULL;
    }
    t->kernel_stack_top = t->kernel_stack + KERNEL_STACK_SIZE;

    *(uint32_t*)t->kernel_stack = DEADCODE_MAGIC;

    t->context.fxsave_area = PTR_TO_U32(kmalloc_aligned(FXSAVE_AREA_SIZE, FXSAVE_AREA_ALIGNMENT));
    if (!t->context.fxsave_area)
    {
        kfree(PTR_FROM_U32(t->kernel_stack));
        kfree(t);
        return NULL;
    }

    memset(PTR_FROM_U32(t->context.fxsave_area), 0, FXSAVE_AREA_SIZE);
    *(uint16_t*)(t->context.fxsave_area + 0) = FCW_DEFAULT;
    *(uint32_t*)(t->context.fxsave_area + 24) = MXCSR_DEFAULT;

    if (!kernel_mode)
    {
        page_directory_t* pd = vmm_create_address_space();
        if (!pd)
        {
            kfree(PTR_FROM_U32(t->kernel_stack));
            kfree(t);
            return NULL;
        }
        t->context.cr3 = PTR_TO_U32(pd);
        t->heap_next = USER_HEAP_BASE;

        const uint32_t user_stack_vaddr = 0xBFFFF000U;
        if (vmm_alloc_page(pd, user_stack_vaddr, PAGE_PRESENT | PAGE_WRITE | PAGE_USER) != 0)
        {
            vmm_destroy_address_space(pd);
            kfree(PTR_FROM_U32(t->kernel_stack));
            kfree(t);
            return NULL;
        }

        t->user_stack = user_stack_vaddr;
        t->user_stack_top = user_stack_vaddr + PAGE_SIZE;
    }

    uint32_t* kstack = PTR_FROM_U32(t->kernel_stack_top);
    kstack[-1] = kernel_mode ? entry_point : FUNC_PTR_TO_U32(user_task_entry);
    kstack[-2] = 0;
    kstack[-3] = 0;
    kstack[-4] = 0;
    kstack[-5] = 0;

    t->context.esp    = PTR_TO_U32(&kstack[-5]);
    t->user_entry     = kernel_mode ? 0 : entry_point;
    t->context.eip    = kernel_mode ? entry_point : FUNC_PTR_TO_U32(user_task_entry);
    t->context.eflags = 0x202;

    t->next = task_queue;
    task_queue = t;

    return t;
}

struct task* task_create(void (*entry)(void), const uint8_t priority, const bool kernel_mode)
{
    const uint32_t flags = spinlock_acquire(&sched_lock);
    struct task* t = task_alloc(FUNC_PTR_TO_U32(entry), priority, kernel_mode, TASK_READY);
    spinlock_release(&sched_lock, flags);
    return t;
}

struct task* task_create_user(const uint32_t entry_point, const uint8_t priority)
{
    const uint32_t flags = spinlock_acquire(&sched_lock);
    struct task* t = task_alloc(entry_point, priority, false, TASK_BLOCKED);
    spinlock_release(&sched_lock, flags);
    return t;
}

void task_destroy(const tid_t id)
{
    //log_info_fmt("task_destroy called for id=%u at tick=%u", id, tick_count);
    const uint32_t flags = spinlock_acquire(&sched_lock);
    struct task* prev = NULL;
    struct task* t = task_queue;

    while (t)
    {
        if (t->id == id)
        {
            if (prev) prev->next = t->next;
            else task_queue = t->next;
            t->next = NULL;

            if (t->kernel_stack) kfree(PTR_FROM_U32(t->kernel_stack));
            if (t->context.fxsave_area) kfree_aligned(PTR_FROM_U32(t->context.fxsave_area));
            fs_process_cleanup(t->pid);
            pty_process_cleanup(t->pid);
            shm_process_cleanup(t->pid);
            ipc_process_cleanup(t->pid);
            if (!t->kernel_mode && t->context.cr3)
            {
                vmm_destroy_address_space(PTR_FROM_U32_TYPED(page_directory_t, t->context.cr3));
            }
            kfree(t);
            spinlock_release(&sched_lock, flags);
            return;
        }
        prev = t;
        t = t->next;
    }
    spinlock_release(&sched_lock, flags);
}

void task_exit(const tid_t id, const int32_t exit_code)
{
    const uint32_t flags = spinlock_acquire(&sched_lock);
    struct task* t = task_queue;
    while (t)
    {
        if (t->id == id)
        {
            t->state = TASK_ZOMBIE;
            t->exit_code = exit_code;
            t->exit_tick = tick_count;

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
                    child->parent_pid = reaper_pid;
                }
                child = child->next;
            }

            spinlock_release(&sched_lock, flags);
            return;
        }
        t = t->next;
    }
    spinlock_release(&sched_lock, flags);
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

pid_t task_fork(struct registers* regs)
{
    if (!current_task)
    {
        return -1;
    }

    struct task* child = kmalloc(sizeof(struct task));
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
    child->window_ticks = 0;
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

    child->context.fxsave_area = PTR_TO_U32(kmalloc_aligned(FXSAVE_AREA_SIZE, FXSAVE_AREA_ALIGNMENT));
    if (!child->context.fxsave_area)
    {
        kfree(child);
        return -1;
    }
    memcpy(PTR_FROM_U32(child->context.fxsave_area), PTR_FROM_U32(current_task->context.fxsave_area), FXSAVE_AREA_SIZE);

    *(uint32_t*)child->kernel_stack = DEADCODE_MAGIC;

    const uint32_t parent_regs_offset = PTR_TO_U32(regs) - current_task->kernel_stack;
    struct registers* child_regs = PTR_FROM_U32_TYPED(struct registers, child->kernel_stack + parent_regs_offset);

    child_regs->eax = 0;
    child->context.eip = FUNC_PTR_TO_U32(isr_fork_resume);
    child->context.esp = child->kernel_stack + parent_regs_offset;

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
    fs_process_fork(current_task->pid, child->pid);
    shm_process_fork(current_task->pid, child->pid);

    return child->pid;
}

pid_t task_wait(const pid_t pid, int32_t* status)
{
    return task_wait_ex(pid, status, false);
}

pid_t task_wait_ex(const pid_t pid, int32_t* status, const bool nohang)
{
    if (!current_task)
    {
        return -1;
    }

    while (1)
    {
        const struct task* t = task_queue;
        while (t)
        {
            if (t->parent_pid == current_task->pid)
            {
                if ((pid == -1 || t->pid == pid) && t->state == TASK_ZOMBIE)
                {
                    const pid_t child_pid = t->pid;
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

        if (nohang)
        {
            return 0;
        }

        current_task->waiting_for = pid;
        current_task->state = TASK_BLOCKED;
        schedule();
    }
}

int task_kill(const pid_t pid, const int32_t status)
{
    struct task* target = task_find(pid);
    if (!target || target->pid <= 2 || target->kernel_mode)
    {
        return -1;
    }
    task_exit(target->id, status);
    return 0;
}

const char* task_state_to_string(const task_state_t state)
{
    switch (state)
    {
#define X(state_enum, state_str) case state_enum: return state_str;
        TASK_STATES
#undef X
        default: return "UNKNOWN";
    }
}

const char* task_priority_to_string(const uint8_t priority)
{
    switch (priority)
    {
#define X(priority_enum, priority_str) case priority_enum: return priority_str;
        TASK_PRIORITIES
#undef X
        default:
            return "UNKNOWN";
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
    const uint32_t flags = spinlock_acquire(&sched_lock);

    static volatile bool in_schedule = false;
    if (in_schedule)
    {
        spinlock_release(&sched_lock, flags);
        return;
    }
    in_schedule = true;

    struct task* next = pick_next_task();
    if (!next)
    {
        in_schedule = false;
        spinlock_release(&sched_lock, flags);
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
    current_task->context.cr3 = current_task->kernel_mode ? PTR_TO_U32(vmm_get_kernel_directory()) : current_task->context.cr3;

    // ToDo AdrGos: This fixes issues with mkdir and ls in RAM mode in kernel shell.
    // Might as well clean this up later, but for now it works as intended.
    if (current_task->context.cr3)
    {
        current_task->context.cr3 = current_task->kernel_mode ? PTR_TO_U32(vmm_get_kernel_directory()) : current_task->context.cr3;
        vmm_switch_address_space(PTR_FROM_U32_TYPED(page_directory_t, current_task->context.cr3));
    }

    if (current_task->kernel_stack)
    {
        tss_set_kernel_stack(current_task->kernel_stack + KERNEL_STACK_SIZE);
    }

    // release before context switch, the lock MUST NOT BE HELD
    in_schedule = false;
    spinlock_release(&sched_lock, flags);

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
        ASSERT_FMT(*(uint32_t*)current_task->kernel_stack == DEADCODE_MAGIC,
                    "Stack overflow detected for task %u (%s)",
                    current_task->pid,
                    current_task->name);

        current_task->cpu_ticks++;
        current_task->window_ticks++;

        if (current_task->time_slice > 0)
        {
            current_task->time_slice--;
        }

        if (current_task->time_slice == 0)
        {
            schedule();
        }
    }

    if (tick_count - window_start_tick >= CPU_STATS_WINDOW_TICKS)
    {
        struct task* wt = task_queue;
        while (wt)
        {
            wt->window_ticks = 0;
            wt = wt->next;
        }
        window_start_tick = tick_count;
    }

    /* Age all ready tasks so they eventually get CPU time */
    struct task* t = task_queue;
    while (t)
    {
        if (t->state == TASK_READY && t->age < UINT32_INVALID_UNSIGNED)
        {
            t->age++;
        }

        if (t->state == TASK_BLOCKED && t->wake_at_tick != 0 && tick_count >= t->wake_at_tick)
        {
            t->wake_at_tick = 0;
            t->state = TASK_READY;
        }
        t = t->next;
    }
}

struct task* sched_get_current(void)
{
    return current_task;
}

void sched_block(const block_reason_t reason)
{
    if (!current_task)
    {
        return;
    }

    switch (reason)
    {
        case BLOCK_WAITING:
            log_info_fmt("[sched] Blocking current task (reason: waiting for PID %p)\n", current_task->next);
            LAMBDA(void, (void), {
                if (current_task)
                {
                    current_task->waiting_for = -1;
                }
            })();
            break;
        case BLOCK_SLEEPING:
            log_info_fmt("[sched] Blocking current task (reason: sleeping for %p ticks)\n", current_task ? current_task->next : 0);
            LAMBDA(void, (void), {
                if (current_task)
                {
                    current_task->wake_at_tick = (uint32_t) (tick_count + current_task->next);
                }
            })();
            break;
        case BLOCK_IO:
            log_info_fmt("[sched] Blocking current task (reason: I/O, PID %d)\n", current_task ? current_task->pid : 0);
            LAMBDA(void, (void), {
                if (current_task)
                {
                    current_task->waiting_for = -1;
                }
            })();
            break;
    }

    current_task->state = TASK_BLOCKED;
    schedule();
}

void sched_unblock(const tid_t id)
{
    const uint32_t flags = spinlock_acquire(&sched_lock);
    struct task* t = task_queue;
    while (t)
    {
        if (t->id == id)
        {
            t->state = TASK_READY;
            spinlock_release(&sched_lock, flags);
            return;
        }
        t = t->next;
    }
    spinlock_release(&sched_lock, flags);
}

void sched_sleep(const uint32_t ticks)
{
    if (!current_task)
    {
        return;
    }

    current_task->wake_at_tick = tick_count + ticks;
    current_task->state = TASK_BLOCKED;
    schedule();
}

void sched_reap_zombies(void)
{
    const uint32_t flags = spinlock_acquire(&sched_lock);

    tid_t to_reap[16];
    uint32_t reap_count = 0;

    const struct task* t = task_queue;
    while (t && reap_count < 16)
    {
        const struct task* next = t->next;

        if (t->state == TASK_ZOMBIE)
        {
            const struct task* parent = task_find(t->parent_pid);
            const bool parent_gone = (parent == NULL);
            const bool grace_expired = (t->exit_tick != 0) && (tick_count >= t->exit_tick + ZOMBIE_REAP_GRACE_TICKS);
            if (parent_gone || grace_expired)
            {
                to_reap[reap_count++] = t->id;
            }
        }

        t = next;
    }

    spinlock_release(&sched_lock, flags);

    for (uint32_t i = 0; i < reap_count; i++)
    {
        task_destroy(to_reap[i]);
    }
}

uint32_t sched_get_total_ticks(void)
{
    return tick_count;
}

uint32_t sched_get_window_ticks(void)
{
    return tick_count - window_start_tick;
}

void sched_set_reaper(const pid_t pid)
{
    if (task_find(pid))
    {
        reaper_pid = pid;
    }
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
