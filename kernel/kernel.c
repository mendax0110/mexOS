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
#include "core/initrd.h"
#include "core/syscall.h"
#include "drivers/input/keyboard.h"
#include "core/userland.h"
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
#include "include/addr.h"
#include "include/assert.h"
#include "core/rollback.h"
#include "drivers/input/mouse.h"
#include "mm/alloc_track.h"
#include "perm/perm.h"
#include "core/pty.h"
#include "core/shm.h"
#if CONFIG_KERNEL_SHELL
#include "apps/shell.h"
#endif

extern uint32_t _kernel_end;
static uint8_t kernel_heap_mem[KERNEL_HEAP_SIZE] ALIGNED(4096);

static kernel_user_map g_user_map;
static kernel_user_id root_user = { "root", "password", 0, 0 };
static kernel_user_id alice_user = { "adrian", "password", 1000, 0 };
static kernel_group_id root_group = { "root", 0 };
static kernel_group_id alice_group = { "adrian", 100 };
static bool desktop_mode_requested = false;
static bool kernel_console_requested = false;

#define MULTIBOOT_INFO_CMDLINE  (1U << 2)

/**
 * @brief Check for a whitespace-delimited token in the Multiboot command line.
 */
static bool boot_has_option(const uint32_t mboot_info, const char* option)
{
    if (!mboot_info || !option || !*option)
    {
        return false;
    }

    const uint32_t* info = PTR_FROM_U32_TYPED(const uint32_t, mboot_info);
    if (!(info[0] & MULTIBOOT_INFO_CMDLINE) || info[4] == 0)
    {
        return false;
    }

    const char* command_line = PTR_FROM_U32_TYPED(const char, info[4]);
    const size_t option_length = strlen(option);

    while (*command_line)
    {
        while (*command_line == ' ' || *command_line == '\t')
        {
            command_line++;
        }

        const char* token = command_line;
        while (*command_line && *command_line != ' ' && *command_line != '\t')
        {
            command_line++;
        }

        if ((size_t)(command_line - token) == option_length &&
            strncmp(token, option, option_length) == 0)
        {
            return true;
        }
    }

    return false;
}


NORETURN static void idle_task(void)
{
    while (1)
    {
        sched_reap_zombies();
        hlt();
    }
}

NORETURN static void init_task(void)
{
    console_write("[init] Init task started\n");
    console_write("[init] mexOS microkernel v0.1\n");
    console_write("[init] IPC and scheduling ready\n");

#if CONFIG_KERNEL_SHELL
    if (kernel_console_requested)
    {
        console_write("[init] Starting kernel recovery console\n");
        shell_run();
    }
#endif

    if (userland_spawn_init(VTERM_CONSOLE, desktop_mode_requested))
    {
        const struct task* current = sched_get_current();
        if (current)
        {
            task_exit(current->id, 0);
        }
        schedule();

        while (1)
        {
            hlt();
        }
    }

    kernel_panic("Failed to launch userland init process");
}

NORETURN static void selftest_task(void)
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
        LET_TIME_PASS(50000000);
    }
}

void kernel_main(const uint32_t mboot_magic, const uint32_t mboot_info)
{
    console_init();
    desktop_mode_requested =
        mboot_magic == 0x2BADB002 &&
        boot_has_option(mboot_info, "desktop") &&
        !boot_has_option(mboot_info, "console");
    kernel_console_requested =
        mboot_magic == 0x2BADB002 &&
        boot_has_option(mboot_info, "kernel-console");

    console_write("mexOS Microkernel\n");
    console_write("=================\n\n");
    if (kernel_console_requested)
    {
        console_write("[boot] Requested session: kernel recovery console\n");
    }
    else
    {
        console_write(desktop_mode_requested
            ? "[boot] Requested session: desktop\n"
            : "[boot] Requested session: user console\n");
    }

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
        log_warn_fmt("Physical Memory Manager initialized with %u bytes of memory", mem_end);

        const uint32_t kernel_size = (PTR_TO_U32(&_kernel_end) - 0x100000 + 0xFFF) & ~0xFFF;
        const uint32_t bitmap_size = (mem_end / PMM_BLOCK_SIZE / PMM_BLOCKS_PER_BYTE + 0xFFF) & ~0xFFF;
        const uint32_t total_reserved = kernel_size + bitmap_size;

        log_warn_fmt("Kernel size: %u bytes, reserving memory region 0x100000 - 0x%x", total_reserved, 0x100000 + total_reserved);
        pmm_deinit_region(0x100000, total_reserved);

        elf_reserve_grub_sections(mboot_info);

        alloc_track_init();

        const void* heap_start = heap_init(PTR_TO_U32(kernel_heap_mem), KERNEL_HEAP_SIZE);
        log_warn_fmt("Kernel heap initialized at %p with size %u bytes", heap_start, KERNEL_HEAP_SIZE);
        ASSERT(heap_start != NULL);

        vmm_init();
    }

    TRY_CTX("core_services", NULL)
    {
        console_write("[boot] Initializing IPC...\n");
        ipc_init();
        pty_init();
        shm_init();

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
        if (desktop_mode_requested && !vesa_is_available())
        {
            console_write("[boot] No usable framebuffer; falling back to console\n");
            desktop_mode_requested = false;
        }

        console_write("[boot] Initializing PCI bus...\n");
        pci_init();

        console_write("[boot] Initializing ACPI...\n");
        acpi_init();

        console_write("[boot] Initializing RTC...\n");
        rtc_init();

        console_write("[boot] Initializing keyboard...\n");
        keyboard_init();

        console_write("[boot] Initializing mouse...\n");
        mouse_init();
    }

    TRY_CTX("storage", LAMBDA(void, (void), {
        fs_sync();
        ahci_shutdown();
        ata_shutdown();
        log_warn("Rolling back storage initialization");
    }))
    {
        console_write("[boot] Initializing ATA disk driver...\n");
        ata_init();

        console_write("[boot] Initializing AHCI SATA driver...\n");
        ahci_init();

        console_write("[boot] Initializing filesystem...\n");
        fs_init();

        if (CONFIG_BOOT_DISK_INSTALLER)
        {
            console_write("[boot] Scanning for storage drives...\n");
            scan_drives();
        }
        else
        {
            console_write("[boot] Disk installer skipped; using RAM filesystem\n");
        }

        console_write("[boot] Installing initrd user programs...\n");
        if (CONFIG_INITRD) // TODO update kconfig to properly handle this
        {
            if (initrd_install() != 0)
            {
                log_warn("Failed to install initrd user programs");
            }
        }
    }

    TRY_CTX("user_management", LAMBDA(void, (void), {
        log_warn("Rolling back user management initialization");
    }))
    {
        console_write("[boot] Initializing user management...\n");

        create_user_map(&g_user_map);

        user_map_add_user(&g_user_map, &root_user);
        user_map_add_user(&g_user_map, &alice_user);

        user_map_add_group(&g_user_map, &root_group);
        user_map_add_group(&g_user_map, &alice_group);

        perm_grant(&root_user, KERNEL_PERM_READ | KERNEL_PERM_WRITE |
                                        KERNEL_PERM_EXEC | KERNEL_PERM_ADMIN);

        perm_grant(&alice_user, KERNEL_PERM_READ | KERNEL_PERM_WRITE);

        perm_set_active_map(&g_user_map);

        if (CONFIG_INITRD)
        {
            set_user_id(&alice_user);
            set_group_id(&alice_group);
        }
        else
        {
            set_user_id(&root_user);
            set_group_id(&root_group);
        }

        console_write("[boot] Logged in as: ");
        console_write(root_user.username);
        console_write("\n");
    }

    TRY_CTX("runtime", LAMBDA(void, (void), {
        timer_disable();
        log_warn("Rolling back runtime initialization");
    }))
    {
        console_write("[boot] Initializing timer...\n");
        timer_init(TICK_FREQUENCY_HZ);

        console_write("[boot] Creating tasks...\n");

        const struct task* idle = task_create(idle_task, TASK_PRIORITY_HIGH, true);
        vterm_set_owner(VTERM_CONSOLE, idle->pid);

        struct task* init = task_create(init_task, TASK_PRIORITY_NORMAL, true);
        init->uid = CONFIG_INITRD ? alice_user.uid : root_user.uid;
        init->gid = CONFIG_INITRD ? alice_group.gid : root_group.gid;
        vterm_set_owner(VTERM_CONSOLE, init->pid);

        if (CONFIG_RUN_SELFTESTS) // TODO check why this does not exec
        {
            const struct task* test = task_create(selftest_task, TASK_PRIORITY_HIGH, true);
            vterm_set_owner(VTERM_USER1, test->pid);
        }
    }

    console_write("[boot] Boot complete!\n\n");
    log_info("Boot sequence complete");

    sti();
    schedule();

    kernel_panic("Scheduler returned!");
}
