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
#include "../servers/console/console.h"
#include "sys/timer.h"
#include "core/syscall.h"
#include "../servers/input/keyboard.h"
#include "../servers/shell/shell.h"
#include "../shared/log.h"
#include "../servers/console/vterm.h"
#include "ui/disk_installer.h"
#include "../servers/block/ata.h"
#include "../servers/block/ahci.h"
#include "drivers/char/rtc.h"
#include "../servers/devmgr/acpi.h"
#include "../servers/devmgr/pci.h"
#include "../servers/console/vesa.h"
#include "../servers/vfs/fs.h"
#include "../tests/test_task.h"
#include "../include/cast.h"

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

void scan_drives(void)
{
    bool has_drives = false;
    for (uint8_t i = 0; i < 4; i++)
    {
        if (ata_drive_exists(i))
        {
            has_drives = true;
            break;
        }
    }

    if (!has_drives)
    {
        for (uint8_t i = 0; i < 32; i++)
        {
            if (ahci_port_exists(i))
            {
                has_drives = true;
                break;
            }
        }
    }

    if (has_drives)
    {
        console_write("[boot] Starting disk installer...\n");
        const int selected_drive = disk_installer_dialog();

        if (selected_drive >= 0)
        {
            if (fs_enable_disk((uint8_t)selected_drive) == 0)
            {
                log_info_fmt("Persistent filesystem enabled on drive %d", selected_drive);
                console_clear();
            }
            else
            {
                log_warn("Failed to enable disk filesystem, using RAM-only mode");
            }
        }
        else
        {
            log_info("Running in RAM-only filesystem mode");
            console_clear();
        }
    }
    else
    {
        console_write("[boot] No storage drives detected\n");
        console_write("[boot] Continuing in RAM-only mode...\n");
        log_warn("No ATA drives found, using RAM-only filesystem");
        for (volatile int i = 0; i < 50000000; i++);
    }
}

//void kernel_main(void)
void kernel_main(const uint32_t mboot_magic, const uint32_t mboot_info)
{
    console_init();
    console_write("mexOS Microkernel\n");
    console_write("=================\n\n");

    log_init();
    log_info("Boot sequence started");

    if (mboot_magic != 0x2BADB002)
    {
        console_write("[warn] Invalid multiboot magic: 0x");
        console_write_hex(mboot_magic);
        console_write("\n");
    }

    console_write("[boot] Initializing GDT...\n");
    gdt_init();
    log_info("GDT initialized");

    console_write("[boot] Initializing IDT...\n");
    idt_init();
    log_info("IDT initialized");

    console_write("[boot] Initializing memory...\n");
    const uint32_t mem_end = 128 * 1024 * 1024;
    pmm_init(mem_end, PTR_TO_U32(&_kernel_end));
    pmm_init_region(0x100000, mem_end - 0x100000);
    log_info("Physical memory manager initialized");

    const uint32_t kernel_size = (PTR_TO_U32(&_kernel_end) - 0x100000 + 0xFFF) & ~0xFFF;
    pmm_deinit_region(0x100000, kernel_size);
    console_write("[boot] Kernel size: ");
    console_write_dec(kernel_size / 1024);
    console_write(" KB reserved\n");
    log_debug("Kernel memory region reserved");

    void* heap_start = heap_init(PTR_TO_U32(kernel_heap_mem), KERNEL_HEAP_SIZE);
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

    console_write("[boot] Initializing framebuffer...\n");
    vesa_init(PTR_FROM_U32(mboot_info));
    log_info("VESA framebuffer initialized");

    console_write("[boot] Initializing PCI bus...\n");
    pci_init();
    log_info("PCI bus enumeration complete");

    console_write("[boot] Initializing ACPI...\n");
    acpi_init();
    log_info("ACPI subsystem initialized");

    console_write("[boot] Initializing RTC...\n");
    rtc_init();
    log_info("RTC driver initialized");

    console_write("[boot] Initializing keyboard...\n");
    keyboard_init();
    log_info("Keyboard driver initialized");

    console_write("[boot] Initializing ATA disk driver...\n");
    ata_init();
    log_info("ATA disk driver initialized");

    console_write("[boot] Initializing AHCI SATA driver...\n");
    ahci_init();
    log_info("AHCI SATA driver initialized");

    console_write("[boot] Initializing filesystem...\n");
    fs_init();

    scan_drives();
    log_info("Filesystem initialized");

    console_write("[boot] Initializing timer...\n");
    timer_init(TICK_FREQUENCY_HZ);
    log_info("Timer initialized");

    console_write("[boot] Creating tasks...\n");
    const struct task* idle = task_create(idle_task, 0, true);
    vterm_set_owner(VTERM_CONSOLE, idle->pid);
    log_debug("Idle task created");
    const struct task* init = task_create(init_task, 1, true);
    vterm_set_owner(VTERM_CONSOLE, init->pid);
    log_debug("Init task created");
    const struct task* test = task_create(selftest_task, 2, true);
    vterm_set_owner(VTERM_USER1, test->pid);
    log_debug("Self-test task created (Alt+F3 to view)");

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
