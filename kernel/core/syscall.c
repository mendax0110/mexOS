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
#include "lib/string.h"
#include "apps/shell.h"
#include "include/addr.h"
#include "shared/syscall_numbers.h"

#define EXEC_MAX_ARGS 16

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
        case SYS_EXEC:
        {
            const char* path = CONST_CHAR_FROM_U32(arg1);
            const char* const* user_argv = (const char* const*)PTR_FROM_U32(arg2);
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
            return msg_send(port_id, msg, arg3);
        }
        case SYS_RECV:
        {
            const int port_id = (int)arg1;
            struct message* msg = PTR_FROM_U32_TYPED(struct message, arg2);
            if (!vmm_check_user_ptr(msg, sizeof(struct message), true)) return -1;
            return msg_receive(port_id, msg, arg3);
        }
        case SYS_PORT_CREATE:
        {
            const struct task* t = sched_get_current();
            return t ? port_create(t->pid) : -1;
        }
        case SYS_PORT_DESTROY:
        {
            return port_destroy((int)arg1);
        }
        case SYS_IOCTL:
        {
            const uint32_t device = arg1;
            const uint32_t request = arg2;
            const void* argp = PTR_FROM_U32(arg3);
            (void)argp; // TODO AdrGos -> use argp

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
            struct vesa_mode_info* info = PTR_FROM_U32_TYPED(struct vesa_mode_info, arg1);
            if (!vmm_check_user_ptr(info, sizeof(struct vesa_mode_info), true)) return -1;
            if (vesa_get_mode_info(info))
            {
                return (int)vesa_get_framebuffer();
            }
            return 0;
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
            const char* line = CONST_CHAR_FROM_U32(arg1);
            char kernel_line[256];
            if (!user_string_ok(line, sizeof(kernel_line))) return -1;
            strncpy(kernel_line, line, sizeof(kernel_line) - 1);
            kernel_line[sizeof(kernel_line) - 1] = '\0';
            execute_command(kernel_line);
            return 0;
        }
        default:
            return -1;
    }
}