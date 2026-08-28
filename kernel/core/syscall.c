#include "syscall.h"
#include "exec/elf.h"
#include "ui/vterm.h"
#include "sched/sched.h"
#include "ipc/ipc.h"
#include "drivers/input/keyboard.h"
#include "drivers/char/rtc.h"
#include "drivers/bus/pci.h"
#include "drivers/video/vesa.h"
#include "fs/fs.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include "include/config.h"
#if CONFIG_KERNEL_SHELL
#include "apps/shell.h"
#endif
#include "include/addr.h"
#include "../shared/syscall_numbers.h"
#include "drivers/input/mouse.h"
#include "core/pty.h"
#include "core/power.h"
#include "perm/perm.h"
#include "../shared/process_abi.h"
#include "../shared/system_abi.h"
#include "core/shm.h"
#include "../shared/io_abi.h"
#include "ui/console.h"
#include "../shared/user_abi.h"

#define EXEC_MAX_ARGS 16
#define USER_FRAMEBUFFER_BASE  0xB0000000U
#define USER_FRAMEBUFFER_LIMIT 0xB8000000U

static pid_t display_owner = -1;

static void syscall_isr(struct registers* regs)
{
    regs->eax = syscall_handler(regs);
}

void syscall_init(void)
{
    register_interrupt_handler(128, syscall_isr);
}

static bool user_string_ok(const char* str, const size_t max_len)
{
    if (!str)
    {
        return false;
    }

    for (size_t i = 0; i < max_len; i++)
    {
        if (!vmm_check_user_ptr(str + i, 1, false))
        {
            return false;
        }
        if (str[i] == '\0')
        {
            return true;
        }
    }

    return false;
}

static int copy_exec_args(const char* path, const char* const* user_argv, const int argc,
                          char kernel_path[FS_MAX_PATH],
                          char kernel_args[EXEC_MAX_ARGS][FS_MAX_PATH],
                          const char* kernel_argv[EXEC_MAX_ARGS + 1],
                          int* out_argc)
{
    if (!path || !kernel_path || !kernel_argv || !out_argc)
    {
        return -1;
    }

    if (!user_string_ok(path, FS_MAX_PATH))
    {
        return -1;
    }

    strncpy(kernel_path, path, FS_MAX_PATH - 1);
    kernel_path[FS_MAX_PATH - 1] = '\0';

    if (!user_argv || argc <= 0)
    {
        strncpy(kernel_args[0], kernel_path, FS_MAX_PATH - 1);
        kernel_args[0][FS_MAX_PATH - 1] = '\0';
        kernel_argv[0] = kernel_args[0];
        kernel_argv[1] = NULL;
        *out_argc = 1;
        return 0;
    }

    if (argc > EXEC_MAX_ARGS)
    {
        return -1;
    }

    if (!vmm_check_user_ptr(user_argv, (size_t)argc * sizeof(const char*), false))
    {
        return -1;
    }

    for (int i = 0; i < argc; i++)
    {
        const char* arg = user_argv[i];
        if (!user_string_ok(arg, FS_MAX_PATH))
        {
            return -1;
        }

        strncpy(kernel_args[i], arg, FS_MAX_PATH - 1);
        kernel_args[i][FS_MAX_PATH - 1] = '\0';
        kernel_argv[i] = kernel_args[i];
    }

    kernel_argv[argc] = NULL;
    *out_argc = argc;
    return 0;
}

static int do_exec(const char* path, const char* const argv[], const int argc, struct registers* regs)
{
    if (!path)
    {
        return -1;
    }

    page_directory_t* new_pd = vmm_create_address_space();
    if (!new_pd)
    {
        return -1;
    }

    struct elf_load_result elf_result;
    uint32_t user_stack_base = 0;
    uint32_t user_stack_top = 0;
    if (elf_load_program(path, new_pd, argc, argv, &elf_result, &user_stack_base, &user_stack_top) != 0)
    {
        vmm_destroy_address_space(new_pd);
        return -1;
    }

    struct task* current = sched_get_current();
    if (!current)
    {
        vmm_destroy_address_space(new_pd);
        return -1;
    }

    page_directory_t* old_pd = (page_directory_t*)current->context.cr3;

    current->context.eip = elf_result.entry_point;
    current->context.cr3 = (uintptr_t)new_pd;
    current->kernel_mode = false;
    current->user_stack = user_stack_base;
    current->user_stack_top = user_stack_top;
    current->user_entry = elf_result.entry_point;
    const char* base = path;
    for (const char* p = path; *p; p++) if (*p == '/') base = p + 1;
    strncpy(current->name, base, sizeof(current->name) - 1);
    current->name[sizeof(current->name) - 1] = '\0';

    if (regs)
    {
        regs->eip = elf_result.entry_point;
        regs->useresp = user_stack_top;
    }

    vmm_switch_address_space(new_pd);

    if (old_pd && old_pd != new_pd)
    {
        vmm_destroy_address_space(old_pd);
    }

    return 0;
}

static uint32_t map_framebuffer_to_user(struct vesa_mode_info* info)
{
    if (!info || info->framebuffer == 0 || info->framebuffer_size == 0)
    {
        return 0;
    }

    page_directory_t* page_dir = vmm_get_current_directory();
    if (!page_dir)
    {
        return 0;
    }

    const uint32_t phys_base = info->framebuffer & ~(PAGE_SIZE - 1U);
    const uint32_t page_offset = info->framebuffer - phys_base;
    if (info->framebuffer_size > (~0U - page_offset))
    {
        return 0;
    }

    const uint32_t map_size = info->framebuffer_size + page_offset;
    if (map_size > (~0U - (PAGE_SIZE - 1U)))
    {
        return 0;
    }

    const uint32_t map_bytes = (map_size + PAGE_SIZE - 1U) & ~(PAGE_SIZE - 1U);
    const uint32_t max_map_bytes = USER_FRAMEBUFFER_LIMIT - USER_FRAMEBUFFER_BASE;
    if (map_bytes == 0 || map_bytes > max_map_bytes)
    {
        return 0;
    }

    for (uint32_t offset = 0; offset < map_bytes; offset += PAGE_SIZE)
    {
        if (vmm_map_page(page_dir,
                         USER_FRAMEBUFFER_BASE + offset,
                         phys_base + offset,
                         PAGE_PRESENT | PAGE_WRITE | PAGE_USER | PAGE_CACHE_DISABLE) != 0)
        {
            return 0;
        }
    }

    info->framebuffer = USER_FRAMEBUFFER_BASE + page_offset;
    return info->framebuffer;
}

static uint32_t map_anon_to_user(const uint32_t size)
{
    struct task* current = sched_get_current();
    if (!current || current->kernel_mode)
    {
        return 0;
    }

    if (size == 0 || size > (~0U - (PAGE_SIZE - 1U)))
    {
        return 0;
    }

    const uint32_t map_bytes = (size + PAGE_SIZE - 1U) & ~(PAGE_SIZE - 1U);

    if (current->heap_next > USER_HEAP_LIMIT ||
        map_bytes > USER_HEAP_LIMIT - current->heap_next)
    {
        return 0;
    }

    page_directory_t* page_dir = vmm_get_current_directory();
    if (!page_dir)
    {
        return 0;
    }

    const uint32_t page_count = map_bytes / PAGE_SIZE;
    void* phys = pmm_alloc_blocks(page_count);
    if (!phys)
    {
        return 0;
    }

    const uint32_t virt_base = current->heap_next;
    const uint32_t phys_base = (uint32_t)(uintptr_t)phys;

    for (uint32_t offset = 0; offset < map_bytes; offset += PAGE_SIZE)
    {
        if (vmm_map_page(page_dir,
                        virt_base + offset,
                        phys_base + offset,
                        PAGE_PRESENT | PAGE_WRITE | PAGE_USER) != 0)
        {
            for (uint32_t rollback = 0; rollback < offset; rollback += PAGE_SIZE)
            {
                vmm_unmap_page(page_dir, virt_base + rollback);
            }
            pmm_free_blocks(phys, page_count);
            return 0;
        }
    }

    current->heap_next = virt_base + map_bytes;
    return virt_base;
}

static int unmap_anon_from_user(const uint32_t address, const uint32_t size)
{
    if (address < USER_HEAP_BASE || address >= USER_HEAP_LIMIT || size == 0) return -1;
    if (size > USER_HEAP_LIMIT - address) return -1;
    const uint32_t start = address & ~(PAGE_SIZE - 1U);
    const uint32_t end = (address + size + PAGE_SIZE - 1U) & ~(PAGE_SIZE - 1U);
    page_directory_t* directory = vmm_get_current_directory();
    for (uint32_t page = start; page < end; page += PAGE_SIZE)
    {
        const uint32_t flags = vmm_get_page_flags(directory, page);
        const uint32_t physical = vmm_get_physical_address(directory, page) & ~(PAGE_SIZE - 1U);
        if (!(flags & PAGE_PRESENT) || (flags & PAGE_SHARED)) return -1;
        vmm_unmap_page(directory, page);
        pmm_free_block(PTR_FROM_U32(physical));
    }
    return 0;
}

int syscall_handler(const struct registers* regs)
{
    const uint32_t syscall_num = regs->eax;
    const uint32_t arg1 = regs->ebx;
    const uint32_t arg2 = regs->ecx;
    const uint32_t arg3 = regs->edx;

    switch (syscall_num)
    {
        case SYS_EXIT:
        {
            const struct task* t = sched_get_current();
            if (t)
            {
                task_exit(t->id, (int32_t)arg1);
            }
            schedule();
            return 0;
        }
        case SYS_WRITE:
        {
            const int fd = (int)arg1;
            const char* str = CONST_CHAR_FROM_U32(arg2);
            const uint32_t len = arg3;
            if (!vmm_check_user_ptr((void*)str, len, false)) return -1;

            if (fs_fd_is_open(fd))
            {
                return fs_write_fd(fd, str, len);
            }

            if ((fd == 1 || fd == 2) && sched_get_current() &&
                sched_get_current()->stdout_pty != TASK_PTY_NONE)
            {
                return pty_slave_write(sched_get_current()->stdout_pty, str, len);
            }

            if (fd == 1 || fd == 2)
            {
                const struct task* t = sched_get_current();
                const int term_id = t ? vterm_get_by_pid(t->pid) : -1;
                struct vterm* vt = (term_id >= 0) ? vterm_get(term_id) : vterm_get_active();

                for (uint32_t i = 0; i < len; i++)
                {
                    vterm_putchar(vt, str[i]);
                }
                return (int)len;
            }

            return fs_write_fd(fd, str, len);
        }
        case SYS_READ:
        {
            const int fd = (int)arg1;
            char* buf = CHAR_FROM_U32(arg2);
            const uint32_t len = arg3;
            if (!vmm_check_user_ptr(buf, len, true)) return -1;

            if (fs_fd_is_open(fd))
            {
                return fs_read_fd(fd, buf, len);
            }

            if (fd == 0 && sched_get_current() &&
                sched_get_current()->stdin_pty != TASK_PTY_NONE)
            {
                return pty_slave_read(sched_get_current()->stdin_pty, buf, len);
            }

            if (fd != 0)
            {
                return fs_read_fd(fd, buf, len);
            }

            if (len == 0)
            {
                return 0;
            }

            uint32_t count = 0;
            buf[count++] = (char)keyboard_getchar();

            while (count < len && keyboard_has_data())
            {
                buf[count++] = (char)keyboard_getchar();
            }
            return (int)count;
        }
        case SYS_YIELD:
        {
            sched_yield();
            return 0;
        }
        case SYS_GETPID:
        {
            const struct task* t = sched_get_current();
            return t ? t->pid : -1;
        }
        case SYS_FORK:
        {
            return task_fork((struct registers*)regs);
        }
        case SYS_WAIT:
        {
            int32_t status = 0;
            const pid_t result = task_wait((pid_t)arg1, &status);
            if (arg2)
            {
                const void* user_status_ptr = PTR_FROM_U32(arg2);
                if (vmm_check_user_ptr(user_status_ptr, sizeof(int32_t), true))
                {
                    int32_t* p = PTR_FROM_U32_TYPED(int32_t, arg2);
                    *p = status;
                }
            }
            return result;
        }
        case SYS_WAITPID:
        {
            int32_t status = 0;
            const pid_t result = task_wait_ex((pid_t)arg1, &status, (arg3 & WAIT_NOHANG) != 0);
            if (arg2)
            {
                int32_t* user_status = PTR_FROM_U32_TYPED(int32_t, arg2);
                if (!vmm_check_user_ptr(user_status, sizeof(*user_status), true)) return -1;
                *user_status = status;
            }
            return result;
        }
        case SYS_KILL:
        {
            const struct task* caller = sched_get_current();
            if (!caller) return -1;
            if ((int32_t)arg1 < 0)
            {
                const pid_t group = -(pid_t)arg1;
                int killed = 0;
                struct task* candidate = sched_get_task_list();
                while (candidate)
                {
                    if (candidate->process_group == group && !candidate->kernel_mode &&
                        (caller->uid == 0 || caller->uid == candidate->uid ||
                         candidate->parent_pid == caller->pid))
                    {
                        const pid_t pid = candidate->pid;
                        candidate = candidate->next;
                        if (task_kill(pid, (int32_t)arg2) == 0) killed++;
                        continue;
                    }
                    candidate = candidate->next;
                }
                return killed ? 0 : -1;
            }
            const struct task* target = task_find((pid_t)arg1);
            if (!target) return -1;
            if (caller->uid != 0 && caller->uid != target->uid && target->parent_pid != caller->pid)
            {
                return -1;
            }
            return task_kill((pid_t)arg1, (int32_t)arg2);
        }
        case SYS_EXEC:
        {
            const char* path = CONST_CHAR_FROM_U32(arg1);
            const char* const* user_argv = PTR_FROM_U32(arg2);
            char kernel_path[FS_MAX_PATH];
            char kernel_args[EXEC_MAX_ARGS][FS_MAX_PATH];
            const char* kernel_argv[EXEC_MAX_ARGS + 1];
            int argc = 0;

            if (copy_exec_args(path, user_argv, (int)arg3, kernel_path, kernel_args, kernel_argv, &argc) != 0)
            {
                return -1;
            }

            return do_exec(kernel_path, kernel_argv, argc, (struct registers*)regs);
        }
        case SYS_SEND:
        {
            const int port_id = (int)arg1;
            struct message* msg = PTR_FROM_U32_TYPED(struct message, arg2);
            if (!vmm_check_user_ptr(msg, sizeof(struct message), false)) return -1;
            struct message kernel_message = *msg;
            const struct task* caller = sched_get_current();
            kernel_message.sender = caller ? caller->pid : 0;
            return msg_send(port_id, &kernel_message, arg3);
        }
        case SYS_RECV:
        {
            const int port_id = (int)arg1;
            struct message* msg = PTR_FROM_U32_TYPED(struct message, arg2);
            if (!vmm_check_user_ptr(msg, sizeof(struct message), true)) return -1;
            const struct task* caller = sched_get_current();
            if (!caller || !port_owned_by(port_id, caller->pid)) return -1;
            return msg_receive(port_id, msg, arg3);
        }
        case SYS_PORT_CREATE:
        {
            const struct task* t = sched_get_current();
            return t ? port_create(t->pid) : -1;
        }
        case SYS_PORT_DESTROY:
        {
            const struct task* caller = sched_get_current();
            if (!caller || !port_owned_by((int)arg1, caller->pid)) return -1;
            return port_destroy((int)arg1);
        }
        case SYS_IOCTL:
        {
            const uint32_t device = arg1;
            const uint32_t request = arg2;
            const void* argp = PTR_FROM_U32(arg3);
            UNUSED(argp, "use argp in future");

            switch (device)
            {
                case 1:
                {
                    break;
                }
                case 2:
                {
                    if (request == 0)
                    {
                        pci_list_devices();
                        return 0;
                    }
                    break;
                }
                default:
                    break;
            }
            return -1;
        }
        case SYS_MMAP:
        {
            if (!vesa_is_available()) return 0;
            const struct task* caller = sched_get_current();
            if (!caller || caller->pid != display_owner) return 0;
            struct vesa_mode_info* info = PTR_FROM_U32_TYPED(struct vesa_mode_info, arg1);
            if (!vmm_check_user_ptr(info, sizeof(struct vesa_mode_info), true)) return -1;
            if (!vesa_get_mode_info(info))
            {
                return 0;
            }

            return (int)map_framebuffer_to_user(info);
        }
        case SYS_MMAP_ANON:
        {
            return (int)map_anon_to_user(arg1);
        }
        case SYS_GETTIME:
        {
            struct rtc_time* time = PTR_FROM_U32_TYPED(struct rtc_time, arg1);
            if (!vmm_check_user_ptr(time, sizeof(struct rtc_time), true)) return -1;
            rtc_read_time(time);
            return 0;
        }
        case SYS_SETTIME:
        {
            struct rtc_time* time = PTR_FROM_U32_TYPED(struct rtc_time, arg1);
            if (!vmm_check_user_ptr(time, sizeof(struct rtc_time), false)) return -1;
            rtc_write_time(time);
            return 0;
        }
        case SYS_OPEN:
        {
            const char* path = CONST_CHAR_FROM_U32(arg1);
            if (!user_string_ok(path, FS_MAX_PATH)) return -1;
            return fs_open(path, (const int)arg2);
        }
        case SYS_CLOSE:
        {
            return fs_close((const int)arg1);
        }
        case SYS_READDIR:
        {
            const char* path = CONST_CHAR_FROM_U32(arg1);
            struct fs_dirent* entries = PTR_FROM_U32_TYPED(struct fs_dirent, arg2);
            const uint32_t max_entries = arg3;
            if (!user_string_ok(path, FS_MAX_PATH)) return -1;
            if (max_entries == 0 || max_entries > FS_MAX_FILES) return -1;
            if (!vmm_check_user_ptr(entries, (size_t)max_entries * sizeof(struct fs_dirent), true)) return -1;
            return fs_readdir(path, entries, max_entries);
        }
        case SYS_STAT:
        {
            const char* path = CONST_CHAR_FROM_U32(arg1);
            struct fs_stat* info = PTR_FROM_U32_TYPED(struct fs_stat, arg2);
            if (!user_string_ok(path, FS_MAX_PATH)) return -1;
            if (!vmm_check_user_ptr(info, sizeof(struct fs_stat), true)) return -1;
            return fs_stat(path, info);
        }
        case SYS_CHDIR:
        {
            const char* path = CONST_CHAR_FROM_U32(arg1);
            if (!user_string_ok(path, FS_MAX_PATH)) return -1;
            return fs_change_dir(path);
        }
        case SYS_GETCWD:
        {
            char* buffer = PTR_FROM_U32_TYPED(char, arg1);
            const uint32_t size = arg2;
            if (!buffer || size == 0) return -1;
            if (!vmm_check_user_ptr(buffer, size, true)) return -1;
            return fs_get_cwd_copy(buffer, size);
        }
        case SYS_SHELL_EXEC:
        {
#if CONFIG_KERNEL_SHELL
            const char* line = CONST_CHAR_FROM_U32(arg1);
            char kernel_line[256];
            if (!user_string_ok(line, sizeof(kernel_line))) return -1;
            strncpy(kernel_line, line, sizeof(kernel_line) - 1);
            kernel_line[sizeof(kernel_line) - 1] = '\0';
            execute_command(kernel_line);
            return 0;
#else
            return -1;
#endif
        }
        case SYS_POLL_KEY:
        {
            unsigned char* out = PTR_FROM_U32_TYPED(unsigned char, arg1);
            if (!vmm_check_user_ptr(out, sizeof(unsigned char), true)) return -1;
            unsigned char key = 0;
            if (!keyboard_try_getchar(&key))
            {
                return 0;
            }
            *out = key;
            return 1;
        }
        case SYS_POLL_MOUSE:
        {
            struct mouse_state* out = PTR_FROM_U32_TYPED(struct mouse_state, arg1);
            if (!vmm_check_user_ptr(out, sizeof(struct mouse_state), true)) return -1;
            struct mouse_state state;
            mouse_try_get_state(&state);
            *out = state;
            return 1;
        }
        case SYS_PTY_CREATE:
        {
            const struct task* caller = sched_get_current();
            return caller ? pty_create(caller->pid) : -1;
        }
        case SYS_PTY_ATTACH:
        {
            return pty_attach_slave((int)arg1);
        }
        case SYS_PTY_READ:
        {
            char* buffer = PTR_FROM_U32_TYPED(char, arg2);
            if (!vmm_check_user_ptr(buffer, arg3, true)) return -1;
            const struct task* caller = sched_get_current();
            return caller ? pty_master_read((int)arg1, buffer, arg3, caller->pid) : -1;
        }
        case SYS_PTY_WRITE:
        {
            const char* buffer = CONST_CHAR_FROM_U32(arg2);
            if (!vmm_check_user_ptr(buffer, arg3, false)) return -1;
            const struct task* caller = sched_get_current();
            return caller ? pty_master_write((int)arg1, buffer, arg3, caller->pid) : -1;
        }
        case SYS_PTY_DESTROY:
        {
            const struct task* caller = sched_get_current();
            return caller ? pty_destroy((int)arg1, caller->pid) : -1;
        }
        case SYS_DISPLAY_CLAIM:
        {
            const struct task* caller = sched_get_current();
            if (!caller) return -1;
            if (display_owner >= 0 && task_find(display_owner)) return -1;
            const int service_port = port_create(caller->pid);
            if (service_port != 0)
            {
                if (service_port >= 0) port_destroy(service_port);
                return -1;
            }
            display_owner = caller->pid;
            return service_port;
        }
        case SYS_POWER:
        {
            const struct task* caller = sched_get_current();
            if (!caller)
            {
                return -1;
            }
            const bool is_desktop = strcmp(caller->name, "desktop") == 0;
            const bool is_admin = current_user_has_perm(KERNEL_PERM_ADMIN) == 0;
            if (!is_admin && !is_desktop)
            {
                return -1;
            }
            if (arg1 == POWER_REBOOT) kernel_power_reboot();
            if (arg1 == POWER_SHUTDOWN) kernel_power_shutdown();
            return -1;
        }
        case SYS_GETPROCS:
        {
            struct process_info* output = PTR_FROM_U32_TYPED(struct process_info, arg1);
            const uint32_t capacity = arg2;
            if (!output || capacity == 0 || !vmm_check_user_ptr(output, capacity * sizeof(*output), true)) return -1;
            uint32_t count = 0;
            const struct task* task = sched_get_task_list();
            while (task && count < capacity)
            {
                output[count].pid = task->pid;
                output[count].parent_pid = task->parent_pid;
                output[count].session_id = task->session_id;
                output[count].uid = task->uid;
                output[count].state = task->state;
                output[count].cpu_ticks = task->cpu_ticks;
                strncpy(output[count].name, task->name, PROCESS_NAME_MAX - 1);
                output[count].name[PROCESS_NAME_MAX - 1] = '\0';
                count++;
                task = task->next;
            }
            return (int)count;
        }
        case SYS_GETUID:
        {
            const struct task* caller = sched_get_current();
            return caller ? (int)caller->uid : -1;
        }
        case SYS_GETUSER:
        {
            struct user_info* out = PTR_FROM_U32_TYPED(struct user_info, arg1);
            if (!vmm_check_user_ptr(out, sizeof(*out), true)) return -1;

            const kernel_user_id* uid = get_current_user();
            if (!uid) return -1;

            out->uid = uid->uid;
            out->is_admin = current_user_has_perm(KERNEL_PERM_ADMIN) ? 1 : 0;
            memset(out->username, 0, USER_NAME_MAX);
            strncpy(out->username, uid->username, USER_NAME_MAX - 1);

            return 0;
        }
        case SYS_FS_MUTATE:
        {
            const char* path = CONST_CHAR_FROM_U32(arg2);
            if (arg1 != FS_OP_SYNC && !user_string_ok(path, FS_MAX_PATH)) return -1;
            switch (arg1)
            {
                case FS_OP_MKDIR: return fs_create_dir(path);
                case FS_OP_TOUCH: return fs_create_file(path);
                case FS_OP_REMOVE: return fs_remove(path);
                case FS_OP_SYNC: return fs_sync();
                default: return -1;
            }
        }
        case SYS_UPTIME:
        {
            return (int)sched_get_total_ticks();
        }
        case SYS_SYSINFO:
        {
            struct system_info* info = PTR_FROM_U32_TYPED(struct system_info, arg1);
            if (!vmm_check_user_ptr(info, sizeof(*info), true)) return -1;
            info->total_memory_kb = pmm_get_block_count() * 4U;
            info->free_memory_kb = pmm_get_free_block_count() * 4U;
            info->used_memory_kb = info->total_memory_kb - info->free_memory_kb;
            info->uptime_ticks = sched_get_total_ticks();
            return 0;
        }
        case SYS_SHM_CREATE:
        {
            const struct task* caller = sched_get_current();
            return caller ? shm_create(arg1, caller->pid) : -1;
        }
        case SYS_SHM_MAP:
        {
            const struct task* caller = sched_get_current();
            return caller ? (int)shm_map((int)arg1, caller->pid) : 0;
        }
        case SYS_SHM_DETACH:
        {
            const struct task* caller = sched_get_current();
            return caller ? shm_detach((int)arg1, caller->pid) : -1;
        }
        case SYS_SHM_DESTROY:
        {
            const struct task* caller = sched_get_current();
            return caller ? shm_destroy((int)arg1, caller->pid) : -1;
        }
        case SYS_PIPE:
        {
            int* fds = PTR_FROM_U32_TYPED(int, arg1);
            if (!vmm_check_user_ptr(fds, sizeof(int) * 2U, true)) return -1;
            return fs_pipe(fds);
        }
        case SYS_DUP2:
        {
            return fs_dup2((int)arg1, (int)arg2);
        }
        case SYS_POLL_FD:
        {
            return fs_poll_fd((int)arg1, (int)arg2);
        }
        case SYS_SETPGID:
        {
            struct task* target = arg1 == 0 ? sched_get_current() : task_find((pid_t)arg1);
            const struct task* caller = sched_get_current();
            if (!target || !caller || (target != caller && target->parent_pid != caller->pid)) return -1;
            target->process_group = arg2 == 0 ? target->pid : (pid_t)arg2;
            return 0;
        }
        case SYS_GETPGID:
        {
            const struct task* target = arg1 == 0 ? sched_get_current() : task_find((pid_t)arg1);
            return target ? target->process_group : -1;
        }
        case SYS_MUNMAP:
        {
            return unmap_anon_from_user(arg1, arg2);
        }
        default:
            return -1;
    }
}
