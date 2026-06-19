#include "kernel.h"
#include "diag/panic.h"
#include "exec/elf.h"
#include "include/config.h"
#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/arch.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "mm/vmm.h"
#include "sched/sched.h"
#include "ipc/ipc.h"
#include "ui/console.h"
#include "sched/timer.h"
#include "core/syscall.h"
#include "drivers/input/keyboard.h"
#include "apps/shell.h"
#include "lib/log.h"
#include "ui/vterm.h"
#include "apps/disk_installer.h"
#include "drivers/storage/ata.h"
#include "drivers/storage/ahci.h"
#include "drivers/char/rtc.h"
#include "drivers/bus/acpi.h"
#include "drivers/bus/pci.h"
#include "drivers/video/vesa.h"
#include "fs/fs.h"
#include "../tests/sched/test_task.h"
#include "../include/cast.h"

extern uint32_t _kernel_end;
static uint8_t kernel_heap_mem[KERNEL_HEAP_SIZE] ALIGNED(4096);

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

void kernel_main(const uint32_t mboot_magic, const uint32_t mboot_info)
{
    console_init();
    console_write("mexOS Microkernel\n");
    console_write("=================\n\n");

    log_init();
    log_info("Boot sequence started");

    TRY_CTX("boot_core", NULL)
    {
        console_write("[boot] Loading kernel symbol table...\n");
        elf_init_symbols(mboot_info);

        ASSERT(mboot_magic == 0x2BADB002);

        console_write("[boot] Initializing GDT...\n");
        gdt_init();

        console_write("[boot] Initializing IDT...\n");
        idt_init();
    }

    TRY_CTX("memory_subsystem", LAMBDA(void, (void), {
        heap_shutdown();
        vmm_shutdown();
        pmm_shutdown();
        log_warn("Rolling back memory initialization");
    }))
    {
        console_write("[boot] Initializing memory...\n");

        const uint32_t mem_end = 128 * 1024 * 1024;
        pmm_init(mem_end, PTR_TO_U32(&_kernel_end));
        pmm_init_region(0x100000, mem_end - 0x100000);

        const uint32_t kernel_size = (PTR_TO_U32(&_kernel_end) - 0x100000 + 0xFFF) & ~0xFFF;

        pmm_deinit_region(0x100000, kernel_size);

        elf_reserve_grub_sections(mboot_info);

        void* heap_start = heap_init(PTR_TO_U32(kernel_heap_mem), KERNEL_HEAP_SIZE);

        ASSERT(heap_start != NULL);

        vmm_init();
    }

    TRY_CTX("core_services", NULL)
    {
        console_write("[boot] Initializing IPC...\n");
        ipc_init();

        console_write("[boot] Initializing scheduler...\n");
        sched_init();

        console_write("[boot] Initializing syscalls...\n");
        syscall_init();
    }

    TRY_CTX("hardware", LAMBDA(void, (void), {
        keyboard_shutdown();
        log_warn("Rolling back hardware initialization");
    }))
    {
        console_write("[boot] Initializing framebuffer...\n");
        vesa_init(PTR_FROM_U32(mboot_info));

        console_write("[boot] Initializing PCI bus...\n");
        pci_init();

        console_write("[boot] Initializing ACPI...\n");
        acpi_init();

        console_write("[boot] Initializing RTC...\n");
        rtc_init();

        console_write("[boot] Initializing keyboard...\n");
        keyboard_init();
    }

    TRY_CTX("storage", LAMBDA(void, (void), {
        fs_sync();
        log_warn("Rolling back storage initialization");
    }))
    {
        console_write("[boot] Initializing ATA disk driver...\n");
        ata_init();

        console_write("[boot] Initializing AHCI SATA driver...\n");
        ahci_init();

        console_write("[boot] Initializing filesystem...\n");
        fs_init();

        scan_drives();
    }

    TRY_CTX("runtime", LAMBDA(void, (void), {
        timer_disable();
        log_warn("Rolling back runtime initialization");
    }))
    {
        console_write("[boot] Initializing timer...\n");
        timer_init(TICK_FREQUENCY_HZ);

        console_write("[boot] Creating tasks...\n");

        const struct task* idle = task_create(idle_task, 0, true);
        vterm_set_owner(VTERM_CONSOLE, idle->pid);

        const struct task* init = task_create(init_task, 1, true);
        vterm_set_owner(VTERM_CONSOLE, init->pid);

        const struct task* test = task_create(selftest_task, 2, true);
        vterm_set_owner(VTERM_USER1, test->pid);
    }

    console_write("[boot] Boot complete!\n\n");
    log_info("Boot sequence complete");

    sti();
    schedule();

    kernel_panic("Scheduler returned!");
}

// C++ support
void __cxa_pure_virtual(void) { kernel_panic("Pure virtual call"); }
int __cxa_guard_acquire(void* g) { (void)g; return 1; }
void __cxa_guard_release(void* g) { (void)g; }
void __cxa_guard_abort(void* g) { (void)g; }
void* __dso_handle = 0;