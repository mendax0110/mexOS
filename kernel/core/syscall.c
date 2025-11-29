#include "syscall.h"
#include "../sched/sched.h"
#include "../ipc/ipc.h"
#include "console.h"
#include "keyboard.h"

static void syscall_isr(struct registers* regs)
{
    regs->eax = syscall_handler(regs);
}

void syscall_init(void)
{
    register_interrupt_handler(128, syscall_isr);
}

int syscall_handler(struct registers* regs)
{
    uint32_t syscall_num = regs->eax;
    uint32_t arg1 = regs->ebx;
    uint32_t arg2 = regs->ecx;
    uint32_t arg3 = regs->edx;

    switch (syscall_num)
    {
        case SYS_EXIT:
        {
            struct task* t = sched_get_current();
            if (t) task_destroy(t->id);
            schedule();
            return 0;
        }
        case SYS_WRITE:
        {
            const char* str = (const char*)arg1;
            uint32_t len = arg2;
            for (uint32_t i = 0; i < len && str[i]; i++)
            {
                console_putchar(str[i]);
            }
            return len;
        }
        case SYS_READ:
        {
            char* buf = (char*)arg1;
            uint32_t len = arg2;
            uint32_t count = 0;
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
            struct task* t = sched_get_current();
            return t ? t->pid : -1;
        }
        case SYS_SEND:
        {
            int port_id = (int)arg1;
            struct message* msg = (struct message*)arg2;
            return msg_send(port_id, msg, arg3);
        }
        case SYS_RECV:
        {
            int port_id = (int)arg1;
            struct message* msg = (struct message*)arg2;
            return msg_receive(port_id, msg, arg3);
        }
        case SYS_PORT_CREATE:
        {
            struct task* t = sched_get_current();
            return t ? port_create(t->pid) : -1;
        }
        case SYS_PORT_DESTROY:
        {
            return port_destroy((int)arg1);
        }
        default:
            return -1;
    }
}
