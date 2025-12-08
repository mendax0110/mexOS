#include "syscall.h"
#include "elf.h"
#include "../fs/fs.h"
#include "../ui/vterm.h"
#include "../sched/sched.h"
#include "../ipc/ipc.h"
#include "../ui/console.h"
#include "../drivers/input/keyboard.h"
#include "../mm/vmm.h"
#include "../mm/pmm.h"
#include "../include/string.h"
#include "../include/config.h"
#include "../drivers/char/rtc.h"
#include "../drivers/bus/acpi.h"
#include "../drivers/bus/pci.h"
#include "../drivers/video/vesa.h"

static void syscall_isr(struct registers* regs)
{
    regs->eax = syscall_handler(regs);
}

void syscall_init(void)
{
    register_interrupt_handler(128, syscall_isr);
}

static int do_exec(const char* path)
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
    if (elf_load_file(path, new_pd, &elf_result) != 0)
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

    uint32_t user_stack_vaddr = 0xBFFFF000;
    if (vmm_alloc_page(new_pd, user_stack_vaddr, PAGE_PRESENT | PAGE_WRITE | PAGE_USER) != 0)
    {
        vmm_destroy_address_space(new_pd);
        return -1;
    }

    current->context.eip = elf_result.entry_point;
    current->context.cr3 = (uint32_t)new_pd;
    current->kernel_mode = false;

    vmm_switch_address_space(new_pd);

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
            struct task* t = sched_get_current();
            if (t)
            {
                task_exit(t->id, (int32_t)arg1);
            }
            schedule();
            return 0;
        }
        case SYS_WRITE:
        {
            const char* str = (const char*)arg1;
            const uint32_t len = arg2;
            if (!vmm_check_user_ptr(str, len, false)) return -1;

            struct task* t = sched_get_current();
            int term_id = t ? vterm_get_by_pid(t->pid) : -1;
            struct vterm* vt = (term_id >= 0) ? vterm_get(term_id) : vterm_get_active();

            for (uint32_t i = 0; i < len && str[i]; i++)
            {
                vterm_putchar(vt, str[i]);
            }
            return len;
        }
        case SYS_READ:
        {
            char* buf = (char*)arg1;
            const uint32_t len = arg2;
            uint32_t count = 0;
            if (!vmm_check_user_ptr(buf, len, true)) return -1;
            while (count < len)
            {
                if (keyboard_has_data())
                {
                    buf[count++] = keyboard_getchar();
                }
                else
                {
                    break;
                }
            }
            return count;
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
            return task_fork();
        }
        case SYS_WAIT:
        {
            int32_t status = 0;
            pid_t result = task_wait((pid_t)arg1, &status);
            if (arg2 && vmm_check_user_ptr((void*)arg2, sizeof(int32_t), true))
            {
                *(int32_t*)arg2 = status;
            }
            return result;
        }
        case SYS_EXEC:
        {
            const char* path = (const char*)arg1;
            if (!vmm_check_user_ptr(path, 1, false)) return -1;
            return do_exec(path);
        }
        case SYS_SEND:
        {
            const int port_id = (int)arg1;
            struct message* msg = (struct message*)arg2;
            if (!vmm_check_user_ptr(msg, sizeof(struct message), false)) return -1;
            return msg_send(port_id, msg, arg3);
        }
        case SYS_RECV:
        {
            const int port_id = (int)arg1;
            struct message* msg = (struct message*)arg2;
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
            void* argp = (void*)arg3;
            (void)argp;
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
            }
            return -1;
        }
        case SYS_MMAP:
        {
            if (!vesa_is_available()) return 0;
            struct vesa_mode_info* info = (struct vesa_mode_info*)arg1;
            if (!vmm_check_user_ptr(info, sizeof(struct vesa_mode_info), true)) return -1;
            if (vesa_get_mode_info(info))
            {
                return vesa_get_framebuffer();
            }
            return 0;
        }
        case SYS_GETTIME:
        {
            struct rtc_time* time = (struct rtc_time*)arg1;
            if (!vmm_check_user_ptr(time, sizeof(struct rtc_time), true)) return -1;
            rtc_read_time(time);
            return 0;
        }
        case SYS_SETTIME:
        {
            struct rtc_time* time = (struct rtc_time*)arg1;
            if (!vmm_check_user_ptr(time, sizeof(struct rtc_time), false)) return -1;
            rtc_write_time(time);
            return 0;
        }
        default:
            return -1;
    }
}
