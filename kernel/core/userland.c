#include "userland.h"
#include "core/initrd.h"
#include "exec/elf.h"
#include "sched/sched.h"
#include "mm/vmm.h"
#include "ui/vterm.h"
#include "lib/string.h"
#include "include/addr.h"

struct task* userland_spawn(const char* path, const int argc, const char* const argv[], const uint8_t terminal_id)
{
    if (!path) return NULL;
    struct task* task = task_create_user(0, TASK_PRIORITY_NORMAL);
    if (!task) return NULL;

    page_directory_t* directory = PTR_FROM_U32_TYPED(page_directory_t, task->context.cr3);
    struct elf_load_result result;
    uint32_t stack_base = 0;
    uint32_t stack_top = 0;
    if (elf_load_program(path, directory, argc, argv, &result, &stack_base, &stack_top) != 0)
    {
        task_destroy(task->id);
        return NULL;
    }
    task->user_entry = result.entry_point;
    task->user_stack = stack_base;
    task->user_stack_top = stack_top;
    const char* base = path;
    for (const char* p = path; *p; p++) if (*p == '/') base = p + 1;
    strncpy(task->name, base, sizeof(task->name) - 1);
    task->name[sizeof(task->name) - 1] = '\0';
    if (terminal_id < VTERM_MAX_COUNT) vterm_set_owner(terminal_id, task->pid);
    task->state = TASK_READY;
    return task;
}

struct task* userland_spawn_init(const uint8_t terminal_id, const bool desktop_mode)
{
    const char* argv[] = {
        INITRD_INIT_PATH,
        desktop_mode ? "--desktop" : "--shell",
        NULL
    };
    struct task* init = userland_spawn(INITRD_INIT_PATH, 2, argv, terminal_id);
    if (init) sched_set_reaper(init->pid);
    return init;
}
