#include "mexKernel.h"
#include "kernelUtils.h"
#include "userlib.h"
#include "memory.h"

extern "C"
{
void kernel_main();
void user_entry();
}

void user_entry()
{
    shell();
}

void kernel_task()
{
    Kernel* kernel = Kernel::instance();
    kernel->terminal().write("Kernel task running...\n");
    kernel->terminal().write("Kernel: mexOS v0.1\n\n");

    kernel->terminal().write("=== Kernel Memory Layout ===\n");

    // Addresses
    kernel->terminal().write("  Kernel task code address : 0x");
    kernel->terminal().write(hex_to_str((uint32_t)&kernel_task));
    kernel->terminal().write("\n");

    kernel->terminal().write("  User stack address       : 0x");
    kernel->terminal().write(hex_to_str((uint32_t)user_stack));
    kernel->terminal().write("\n");

    kernel->terminal().write("  Kernel heap address      : 0x");
    kernel->terminal().write(hex_to_str((uint32_t)kernel_heap));
    kernel->terminal().write("\n");

    // Sizes
    kernel->terminal().write("  Kernel stack size        : ");
    kernel->terminal().write(int_to_str(KERNEL_STACK_SIZE));
    kernel->terminal().write(" bytes\n");

    kernel->terminal().write("  User stack size          : ");
    kernel->terminal().write(int_to_str(USER_STACK_SIZE));
    kernel->terminal().write(" bytes\n");

    kernel->terminal().write("  Kernel heap size         : ");
    kernel->terminal().write(int_to_str(KERNEL_HEAP_SIZE));
    kernel->terminal().write(" bytes\n");

    // Calculated stats
    const uint32_t used_memory = KERNEL_STACK_SIZE + USER_STACK_SIZE;
    const uint32_t free_memory = KERNEL_HEAP_SIZE > used_memory ? KERNEL_HEAP_SIZE - used_memory : 0;

    kernel->terminal().write("  Total allocated memory   : ");
    kernel->terminal().write(int_to_str(KERNEL_HEAP_SIZE + USER_STACK_SIZE + KERNEL_STACK_SIZE));
    kernel->terminal().write(" bytes\n");

    kernel->terminal().write("  Used memory (stack)      : ");
    kernel->terminal().write(int_to_str(used_memory));
    kernel->terminal().write(" bytes\n");

    kernel->terminal().write("  Free memory in heap      : ");
    kernel->terminal().write(int_to_str(free_memory));
    kernel->terminal().write(" bytes\n");

    kernel->terminal().write("  Heap usage               : ");
    kernel->terminal().write(int_to_str(KERNEL_HEAP_SIZE - free_memory));
    kernel->terminal().write(" bytes\n");

    // Scheduler info
    kernel->terminal().write("\n=== Scheduler Tasks ===\n");
    for (uint32_t i = 0; i < kernel->scheduler().getTaskCount(); i++)
    {
        kernel->terminal().write("  Task ");
        kernel->terminal().write(int_to_str(i));
        kernel->terminal().write(" - Priority: ");
        kernel->terminal().write(int_to_str(kernel->scheduler().getTask(i).priority));
        kernel->terminal().write("\n");
    }

    // Memory pool statistics
    kernel->terminal().write("\n=== Memory Pool Statistics ===\n");
    MemoryPool::instance().printMemoryStats();

    kernel->terminal().write("\nCPU: i686 (32-bit)\n");

    kernel->scheduler().yield();
}

extern "C" void kernel_main()
{
    Kernel* kernel = Kernel::instance();
    kernel->initialize();

    volatile uint32_t stack_test = 0x12345678;
    if (stack_test != 0x12345678)
    {
        Kernel::instance()->terminal().write("Stack corruption detected!\n");
        while(1);
    }

    // Set up yield interrupt (IRQ 0x20)
    idt_set_gate(0x20, (uint32_t)yield_handler, 0x08, 0x8E);

    kernel->scheduler().addTask(kernel_task, 1, true);

    // Setup user process
    uint8_t* user_stack = (uint8_t*)kmalloc(4096);
    uint32_t stack_top = (uint32_t)user_stack + 4096;
    kernel->switch_to_userspace(user_entry, stack_top);
    //kernel->scheduler().addTask(user_entry, 2, false);

    kernel->run();
}