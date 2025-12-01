#include "kernel.h"
#include "include/config.h"
#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/arch.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "mm/vmm.h"
#include "sched/sched.h"
#include "ipc/ipc.h"
#include "core/console.h"
#include "core/timer.h"
#include "core/syscall.h"
#include "core/keyboard.h"
#include "core/shell.h"
#include "core/log.h"
#include "core/ata.h"
#include "../tests/test_task.h"

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

static void selftest_task(void)
{
    test_task();
    while (1)
    {
        hlt();
    }
}

void kernel_panic(const char* msg)
{
    cli();
    console_set_color(VGA_WHITE, VGA_RED);
    console_write("\n\n========================================\n");
    console_write("*** KERNEL PANIC ***\n");
    console_write("========================================\n");
    console_write("Error: ");
    console_write(msg);
    console_write("\n\nRegister dump:\n");
    const uint32_t eflags = read_eflags();
    console_write("EFLAGS: 0x");
    console_write_hex(eflags);
    console_write("\nCR0: 0x");
    console_write_hex(read_cr0());
    console_write("\nCR2: 0x");
    console_write_hex(read_cr2());
    console_write("\nCR3: 0x");
    console_write_hex(read_cr3());
    console_write("\n\nSystem halted.\n");
    console_write("========================================\n");
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
    const uint32_t mem_end = 128 * 1024 * 1024;
    pmm_init(mem_end, (uint32_t)&_kernel_end);
    pmm_init_region(0x100000, mem_end - 0x100000);
    log_info("Physical memory manager initialized");

    const uint32_t kernel_size = ((uint32_t)&_kernel_end - 0x100000 + 0xFFF) & ~0xFFF;
    pmm_deinit_region(0x100000, kernel_size);
    console_write("[boot] Kernel size: ");
    console_write_dec(kernel_size / 1024);
    console_write(" KB reserved\n");
    log_debug("Kernel memory region reserved");

    void* heap_start = heap_init((uint32_t)kernel_heap_mem, KERNEL_HEAP_SIZE);
    if (!heap_start)
    {
        kernel_panic("Failed to initialize kernel heap");
    }
    log_info("Kernel heap initialized");

    console_write("[boot] Memory initialized: ");
    console_write_dec(pmm_get_free_block_count() * 4);
    console_write(" KB free (");
    console_write_dec(pmm_get_free_block_count());
    console_write(" blocks)\n");
    log_info("Memory subsystem initialized");

    console_write("[boot] Initializing virtual memory (enabling paging)...\n");
    console_write("[boot] Need 3 blocks for page directory + 2 page tables\n");
    vmm_init();
    log_info("Virtual memory manager initialized");

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

    console_write("[boot] Initializing ATA disk driver...\n");
    ata_init();
    log_info("ATA disk driver initialized");

    console_write("[boot] Initializing timer...\n");
    timer_init(TICK_FREQUENCY_HZ);
    log_info("Timer initialized");

    console_write("[boot] Creating tasks...\n");
    task_create(idle_task, 0, true);
    log_debug("Idle task created");
    task_create(init_task, 1, true);
    log_debug("Init task created");
    task_create(selftest_task, 2, true);
    log_debug("Self-test task created");

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
