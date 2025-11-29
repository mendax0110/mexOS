#include "kernel.h"
#include "include/config.h"
#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/arch.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "sched/sched.h"
#include "ipc/ipc.h"
#include "core/console.h"
#include "core/timer.h"
#include "core/syscall.h"
#include "core/keyboard.h"
#include "core/shell.h"
#include "core/log.h"

extern uint32_t _kernel_end;

static uint8_t kernel_heap_mem[KERNEL_HEAP_SIZE] __attribute__((aligned(4096)));

static void idle_task(void)
{
    while (1)
    {
        hlt();
    }
}

static void init_task(void)
{
    console_write("[init] Init task started\n");
    console_write("[init] mexOS microkernel v0.1\n");
    console_write("[init] IPC and scheduling ready\n");

    shell_run();
}

void kernel_panic(const char* msg)
{
    cli();
    console_set_color(VGA_WHITE, VGA_RED);
    console_write("\n*** KERNEL PANIC ***\n");
    console_write(msg);
    console_write("\nSystem halted.");
    while (1) hlt();
}

void kernel_main(void)
{
    console_init();
    console_write("mexOS Microkernel\n");
    console_write("=================\n\n");

    log_init();
    log_info("Boot sequence started");

    console_write("[boot] Initializing GDT...\n");
    gdt_init();
    log_info("GDT initialized");

    console_write("[boot] Initializing IDT...\n");
    idt_init();
    log_info("IDT initialized");

    console_write("[boot] Initializing memory...\n");
    uint32_t mem_end = 32 * 1024 * 1024;
    pmm_init(mem_end, (uint32_t)&_kernel_end);
    pmm_init_region(0x100000, mem_end - 0x100000);
    log_info("Physical memory manager initialized");

    pmm_deinit_region(0x100000, (uint32_t)&_kernel_end - 0x100000);
    log_debug("Kernel memory region reserved");

    heap_init((uint32_t)kernel_heap_mem, KERNEL_HEAP_SIZE);
    log_info("Kernel heap initialized");

    console_write("[boot] Memory initialized: ");
    console_write_dec(pmm_get_free_block_count() * 4);
    console_write(" KB free\n");

    console_write("[boot] Initializing IPC...\n");
    ipc_init();
    log_info("IPC subsystem initialized");

    console_write("[boot] Initializing scheduler...\n");
    sched_init();
    log_info("Scheduler initialized");

    console_write("[boot] Initializing syscalls...\n");
    syscall_init();
    log_info("Syscall interface initialized");

    console_write("[boot] Initializing keyboard...\n");
    keyboard_init();
    log_info("Keyboard driver initialized");

    console_write("[boot] Initializing timer...\n");
    timer_init(TICK_FREQUENCY_HZ);
    log_info("Timer initialized");

    console_write("[boot] Creating tasks...\n");
    task_create(idle_task, 0, true);
    log_debug("Idle task created");
    task_create(init_task, 1, true);
    log_debug("Init task created");

    console_write("[boot] Boot complete!\n\n");
    log_info("Boot sequence complete");

    sti();
    log_info("Interrupts enabled");
    schedule();

    kernel_panic("Scheduler returned!");
}

// C++ support
void __cxa_pure_virtual(void) { kernel_panic("Pure virtual call"); }
int __cxa_guard_acquire(void* g) { (void)g; return 1; }
void __cxa_guard_release(void* g) { (void)g; }
void __cxa_guard_abort(void* g) { (void)g; }
void* __dso_handle = 0;
