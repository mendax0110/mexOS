#include "mexKernel.h"
#include "kernelUtils.h"
#include "userlib.h"

uint8_t user_stack[4096];

extern "C"
{
void kernel_main();
void user_entry();
}

void user_program()
{
    print("Hello from userspace!\n");
    while(1) {}
}

void user_entry()
{
    user_program();
}

void kernel_task()
{
    Kernel* kernel = Kernel::instance();
    kernel->terminal().write("Kernel task running...\n");
    kernel->terminal().write("Kernel: mexOS v0.1\n");

    kernel->terminal().write("Memory:\n");
    kernel->terminal().write("  - Kernel stack: 0x");
    kernel->terminal().write(hex_to_str((uint32_t)&kernel_task));
    kernel->terminal().write("\n");
    kernel->terminal().write("  - User stack: 0x");
    kernel->terminal().write(hex_to_str((uint32_t)user_stack));
    kernel->terminal().write("\n");

    kernel->terminal().write("Scheduler Tasks:\n");
    for (uint32_t i = 0; i < kernel->scheduler().getTaskCount(); i++)
    {
        kernel->terminal().write("  - Task ");
        kernel->terminal().write(int_to_str(i));
        kernel->terminal().write(": Priority=");
        kernel->terminal().write(int_to_str(kernel->scheduler().getTask(i).priority));
        kernel->terminal().write("\n");
    }

    kernel->terminal().write("CPU: i686 (32-bit)\n");

    while(1) {}
}

extern "C" void kernel_main()
{
    Kernel* kernel = Kernel::instance();
    kernel->initialize();

    kernel->scheduler().addTask(kernel_task, 1);

    kernel->scheduler().addTask(user_entry, 2);

    kernel->run();
}