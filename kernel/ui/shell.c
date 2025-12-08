#include "shell.h"
#include "console.h"
#include "../drivers/input/keyboard.h"
#include "../fs/fs.h"
#include "../lib/log.h"
#include "../core/elf.h"
#include "../core/initrd.h"
#include "vterm.h"
#include "../include/string.h"
#include "../sched/sched.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../mm/vmm.h"
#include "../arch/i686/arch.h"
#include "../sys/timer.h"
#include "../sys/sysmon.h"
#include "../lib/debug_utils.h"
#include "basic.h"
#include "tui.h"
#include "editor.h"
#include "../../tests/test_runner.h"
#include "../../tests/test_task.h"

#define CMD_BUFFER_SIZE 256
#define MAX_ARGS 16
#define EDITOR_MAX_LINES 64
#define EDITOR_LINE_SIZE 80
#define HISTORY_SIZE 32

static char cmd_buffer[CMD_BUFFER_SIZE];
static uint32_t cmd_pos = 0;

static char editor_buf[FS_MAX_FILE_SIZE];
static char editor_line_buf[EDITOR_LINE_SIZE];

static char history[HISTORY_SIZE][CMD_BUFFER_SIZE];
static uint32_t history_count = 0;
static int32_t history_pos = 0;
static char temp_buffer[CMD_BUFFER_SIZE];

static void shell_prompt(void)
{
    console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    console_write("mexOS");
    console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    console_write("> ");
}

static void history_add(const char* cmd)
{
    if (cmd[0] == '\0')
    {
        return;
    }

    if (history_count > 0
        && strcmp(history[(history_count - 1) % HISTORY_SIZE], cmd) == 0)
    {
        return;
    }

    const uint32_t idx = history_count % HISTORY_SIZE;
    strncpy(history[idx], cmd, CMD_BUFFER_SIZE - 1);
    history[idx][CMD_BUFFER_SIZE - 1] = '\0';
    history_count++;
}

static void clear_line(void)
{
    while (cmd_pos > 0)
    {
        console_putchar('\b');
        console_putchar(' ');
        console_putchar('\b');
        cmd_pos--;
    }
}

static void display_buffer(void)
{
    for (uint32_t i = 0; i < cmd_pos; i++)
    {
        console_putchar(cmd_buffer[i]);
    }
}

static int parse_args(char* cmd, char* argv[])
{
    int argc = 0;
    char* p = cmd;

    while (*p && argc < MAX_ARGS)
    {
        while (*p == ' ')
        {
            p++;
        }
        if (*p == '\0')
        {
            break;
        }
        argv[argc++] = p;
        while (*p && *p != ' ')
        {
            p++;
        }
        if (*p)
        {
            *p++ = '\0';
        }
    }
    return argc;
}

static void cmd_help(void)
{
    console_write("Available commands:\n");
    console_write("  help    - Show this help message\n");
    console_write("  clear   - Clear the screen\n");
    console_write("  ps      - List running tasks\n");
    console_write("  kill    - Terminate a task by PID\n");
    console_write("  mem     - Show memory usage\n");
    console_write("  defrag  - Defragment kernel heap\n");
    console_write("  echo    - Echo arguments\n");
    console_write("  uptime  - Show system uptime\n");
    console_write("  ver     - Show version info\n");
    console_write("  ls      - List directory contents\n");
    console_write("  cd      - Change directory\n");
    console_write("  pwd     - Print working directory\n");
    console_write("  cat     - Display file contents\n");
    console_write("  mkdir   - Create a new directory\n");
    console_write("  rm      - Remove a file or directory\n");
    console_write("  rmdir   - Remove an empty directory\n");
    console_write("  touch   - Create an empty file\n");
    console_write("  edit    - Edit a file\n");
    console_write("  write   - Write text to file\n");
    console_write("  log     - Show system log\n");
    console_write("  clcache - Clear filesystem cache\n");
    console_write("  shutdown- Shutdown the system\n");
    console_write("  reboot  - Reboot the system\n");
    console_write("  cpu     - Show CPU Task usage\n");
    console_write("  sysmon  - Show system statistics\n");
    console_write("  trace   - Show function trace\n");
    console_write("  clrtrace- Clear trace buffer\n");
    console_write("  memdump - Dump memory region\n");
    console_write("  registers- Dump CPU registers\n");
    console_write("  basic   - Enter BASIC interpreter\n");
    console_write("  spawn   - Spawn user-mode init process\n");
    console_write("  forktest- Test fork() syscall\n");
    console_write("  tty     - Show current terminal info\n");
    console_write("  tty N   - Switch to terminal N (0-3)\n");
    console_write("  test    - Run unit tests\n");
    console_write("  dash    - Show System Dashboard");
    console_write("Shortcuts:\n");
    console_write("  Alt+F1-F4     - Switch terminals\n");
    console_write("  PageUp/Down   - Scroll terminal history\n");
    console_write("  Alt+Home/End  - Scroll to top/bottom\n");
}

static void cmd_clear(void)
{
    console_clear();
}

static void cmd_ps(void)
{
    console_write("PID  STATE    PRIORITY\n");
    console_write("----------------------\n");

    struct task* t = sched_get_task_list();
    while (t)
    {
        console_write("  ");
        console_write_dec(t->pid);
        console_write("  ");
        switch (t->state)
        {
            case TASK_RUNNING: console_write("RUNNING  "); break;
            case TASK_READY:   console_write("READY    "); break;
            case TASK_BLOCKED: console_write("BLOCKED  "); break;
            case TASK_ZOMBIE:  console_write("ZOMBIE   "); break;
            default:           console_write("UNKNOWN  "); break;
        }
        console_write_dec(t->priority);
        console_write("\n");

        t = t->next;
    }
}

static void cmd_kill(uint8_t pid)
{
    struct task* t = sched_get_task_list();
    while (t)
    {
        if (t->pid == pid)
        {
            task_destroy(t->id);
            console_write("Task ");
            console_write_dec(pid);
            console_write(" terminated.\n");
            return;
        }
        t = t->next;
    }
    console_write("No such task with PID ");
    console_write_dec(pid);
    console_write(".\n");
}

static void cmd_mem(void)
{
    console_write("Physical Memory:\n");
    console_write("  Total blocks: ");
    console_write_dec(pmm_get_block_count());
    console_write("\n  Used blocks:  ");
    console_write_dec(pmm_get_used_block_count());
    console_write("\n  Free blocks:  ");
    console_write_dec(pmm_get_free_block_count());
    console_write("\n  Free memory:  ");
    console_write_dec(pmm_get_free_block_count() * 4);
    console_write(" KB\n");

    uint32_t free_blocks = 0;
    uint32_t largest_free = 0;
    heap_get_fragmentation(&free_blocks, &largest_free);

    console_write("Kernel Heap:\n");
    console_write("  Total: ");
    console_write_dec(heap_get_used() + heap_get_free());
    console_write(" bytes\n  Used:  ");
    console_write_dec(heap_get_used());
    console_write(" bytes\n  Free: ");
    console_write_dec(heap_get_free());
    console_write(" bytes\n  Free blocks: ");
    console_write_dec(free_blocks);
    console_write("\n  Largest free block: ");
    console_write_dec(largest_free);
    console_write(" bytes\n");
}

static void cmd_defrag(void)
{
    uint32_t free_before, largest_before;
    heap_get_fragmentation(&free_before, &largest_before);

    heap_defragment();

    uint32_t free_after, largest_after;
    heap_get_fragmentation(&free_after, &largest_after);

    console_write("Heap defragmentation completed.\n");
    console_write("Free blocks: ");
    console_write_dec(free_before);
    console_write(" -> ");
    console_write_dec(free_after);
    console_write("\nLargest free block: ");
    console_write_dec(largest_before);
    console_write(" -> ");
    console_write_dec(largest_after);
    console_write(" bytes\n");
}

static void cmd_echo(const int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        console_write(argv[i]);
        if (i < argc - 1)
        {
            console_putchar(' ');
        }
    }
    console_putchar('\n');
}

static void cmd_uptime(void)
{
    const uint32_t ticks = timer_get_ticks();
    const uint32_t seconds = ticks / 100;
    const uint32_t minutes = seconds / 60;
    const uint32_t hours = minutes / 60;

    console_write("Uptime: ");
    console_write_dec(hours);
    console_write("h ");
    console_write_dec(minutes % 60);
    console_write("m ");
    console_write_dec(seconds % 60);
    console_write("s\n");
}

static void cmd_version(void)
{
    console_write("mexOS Microkernel v0.1\n");
    console_write("Architecture: i686\n");
}

static void cmd_ls(const int argc, char* argv[])
{
    const char* path = (argc > 1) ? argv[1] : ".";
    char buffer[1024];

    const int ret = fs_list_dir(path, buffer, sizeof(buffer));
    if (ret == FS_ERR_NOT_FOUND)
    {
        console_write("ls: directory not found\n");
        return;
    }
    if (ret == FS_ERR_NOT_DIR)
    {
        console_write("ls: not a directory\n");
        return;
    }
    if (ret == 0)
    {
        console_write("(empty)\n");
        return;
    }

    console_write(buffer);
}

static void cmd_cd(const int argc, char* argv[])
{
    const char* path = (argc > 1) ? argv[1] : "/";

    const int ret = fs_change_dir(path);
    if (ret == FS_ERR_NOT_FOUND)
    {
        console_write("cd: directory not found\n");
        return;
    }
    if (ret == FS_ERR_NOT_DIR)
    {
        console_write("cd: not a directory\n");
        return;
    }
}

static void cmd_pwd(void)
{
    console_write(fs_get_cwd());
    console_write("\n");
}

static void cmd_cat(const int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("cat: missing file operand\n");
        return;
    }

    char buffer[FS_MAX_FILE_SIZE + 1];
    const int ret = fs_read(argv[1], buffer, FS_MAX_FILE_SIZE);

    if (ret == FS_ERR_NOT_FOUND)
    {
        console_write("cat: file not found\n");
        return;
    }
    if (ret == FS_ERR_IS_DIR)
    {
        console_write("cat: is a directory\n");
        return;
    }
    if (ret == 0)
    {
        return;
    }

    buffer[ret] = '\0';
    console_write(buffer);
    if (ret > 0 && buffer[ret - 1] != '\n')
    {
        console_write("\n");
    }
}

static void cmd_mkdir(const int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("mkdir: missing directory name\n");
        return;
    }

    const int ret = fs_create_dir(argv[1]);
    if (ret == FS_ERR_EXISTS)
    {
        console_write("mkdir: directory already exists\n");
        return;
    }
    if (ret == FS_ERR_FULL)
    {
        console_write("mkdir: filesystem full\n");
        return;
    }
    if (ret == FS_ERR_NOT_FOUND)
    {
        console_write("mkdir: parent directory not found\n");
        return;
    }
}

static void cmd_rm(const int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("rm: missing operand\n");
        return;
    }

    const int ret = fs_remove(argv[1]);
    if (ret == FS_ERR_NOT_FOUND)
    {
        console_write("rm: file or directory not found\n");
        return;
    }
    if (ret == FS_ERR_NOT_EMPTY)
    {
        console_write("rm: directory not empty\n");
        return;
    }
    if (ret == FS_ERR_INVALID)
    {
        console_write("rm: cannot remove root directory\n");
        return;
    }
}

static void cmd_rmdir(const int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("rmdir: missing directory name\n");
        return;
    }

    const int ret = fs_remove(argv[1]);
    if (ret == FS_ERR_NOT_FOUND)
    {
        console_write("rmdir: directory not found\n");
        return;
    }
    if (ret == FS_ERR_NOT_EMPTY)
    {
        console_write("rmdir: directory not empty\n");
        return;
    }
    if (ret == FS_ERR_INVALID)
    {
        console_write("rmdir: cannot remove root directory\n");
        return;
    }
}

static void cmd_touch(const int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("touch: missing file operand\n");
        return;
    }

    if (fs_exists(argv[1]))
    {
        return;
    }

    const int ret = fs_create_file(argv[1]);
    if (ret == FS_ERR_FULL)
    {
        console_write("touch: filesystem full\n");
        return;
    }
    if (ret == FS_ERR_NOT_FOUND)
    {
        console_write("touch: parent directory not found\n");
        return;
    }
}

static void cmd_clear_cache(void)
{
    fs_clear_cache();
    log_info("Filesystem cache cleared");
    console_write("Filesystem cache cleared\n");
}

static void cmd_log(void)
{
    log_dump();
}

static void cmd_shutdown(void)
{
    log_info("Shutdown initiated by user");
    console_write("Shutting down...\n");

    log_info("Attempting QEMU ACPI shutdown");
    outw(0x604, 0x2000);

    log_info("Attempting Bochs ACPI shutdown");
    outw(0xB004, 0x2000);

    log_info("Attempting VirtualBox ACPI shutdown");
    outw(0x4004, 0x3400);

    log_warn("ACPI shutdown failed, halting CPU");
    cli();
    console_write("System halted. You may power off now.\n");
    while (1)
    {
        hlt();
    }
}

static void cmd_reboot(void)
{
    log_info("Reboot initiated by user");
    console_write("Rebooting...\n");

    log_info("Waiting for keyboard controller");
    uint8_t status;
    do
    {
        status = inb(0x64);
    } while (status & 0x02);

    log_info("Sending reset command to keyboard controller");
    outb(0x64, 0xFE);

    log_warn("Keyboard reset failed, halting CPU");
    cli();
    while (1)
    {
        hlt();
    }
}

static void cmd_edit(const int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("edit: missing file operand\n");
        return;
    }

    const char* filename = argv[1];
    uint8_t mode = EDITOR_MODE_TEXT;

    const char* ext = filename;
    while (*ext) ext++;
    while (ext > filename && *ext != '.') ext--;

    if (strcmp(ext, ".bas") == 0 || strcmp(ext, ".BAS") == 0)
    {
        mode = EDITOR_MODE_BASIC;
    }

    if (editor_open(filename, mode) == 0)
    {
        editor_run();
    }
}

static void cmd_write(const int argc, char* argv[])
{
    if (argc < 3)
    {
        console_write("write: usage: write <file> <text>\n");
        return;
    }

    const char* filename = argv[1];

    if (!fs_exists(filename))
    {
        const int ret = fs_create_file(filename);
        if (ret != FS_ERR_OK)
        {
            console_write("write: cannot create file\n");
            return;
        }
    }
    else if (fs_is_dir(filename))
    {
        console_write("write: is a directory\n");
        return;
    }

    char content[FS_MAX_FILE_SIZE];
    uint32_t pos = 0;

    for (int i = 2; i < argc && pos < FS_MAX_FILE_SIZE - 2; i++)
    {
        const uint32_t len = (uint32_t)strlen(argv[i]);
        if (pos + len + 1 >= FS_MAX_FILE_SIZE)
        {
            break;
        }
        if (i > 2)
        {
            content[pos++] = ' ';
        }
        memcpy(content + pos, argv[i], len);
        pos += len;
    }
    content[pos++] = '\n';
    content[pos] = '\0';

    fs_write(filename, content, pos);
}

static void cmd_cpu(void)
{
    uint32_t total_ticks = sched_get_total_ticks();
    if (total_ticks == 0)
    {
        console_write("No CPU data yet.\n");
        return;
    }

    console_write("PID  CPU%   STATE\n");
    console_write("------------------\n");

    struct task* t = sched_get_task_list();
    while (t)
    {
        uint32_t cpu_percent = 0;

        if (t->cpu_ticks > 0)
        {
            cpu_percent = (t->cpu_ticks * 100) / total_ticks;
        }

        console_write_dec(t->pid);
        console_write("   ");

        console_write_dec(cpu_percent);
        console_write("%   ");

        switch (t->state)
        {
            case TASK_RUNNING: console_write("RUNNING"); break;
            case TASK_READY:   console_write("READY"); break;
            case TASK_BLOCKED: console_write("BLOCKED"); break;
            case TASK_ZOMBIE:  console_write("ZOMBIE"); break;
            default:           console_write("UNKNOWN"); break;
        }

        console_write("\n");
        t = t->next;
    }

    struct task* idle = sched_get_idle_task();
    if (idle)
    {
        uint32_t idle_percent = (idle->cpu_ticks * 100) / total_ticks;

        console_write("\nTotal CPU used: ");
        console_write_dec(100 - idle_percent);
        console_write("%\n");
    }
}

static void cmd_sysmon(void)
{
    sysmon_print_summary();
}

static void cmd_trace(void)
{
    debug_print_trace();
}

static void cmd_clear_trace(void)
{
    debug_clear_trace();
}

static void cmd_memdump(const int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("memdump: usage: memdump <address> [count]\n");
        return;
    }

    uint32_t addr = 0;
    for (size_t i = 0; argv[1][i] != '\0'; i++)
    {
        char c = argv[1][i];
        if (c >= '0' && c <= '9')
        {
            addr = addr * 16 + (c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            addr = addr * 16 + (c - 'a' + 10);
        }
        else if (c >= 'A' && c <= 'F')
        {
            addr = addr * 16 + (c - 'A' + 10);
        }
    }

    uint32_t count = 16;
    if (argc >= 3)
    {
        count = 0;
        for (size_t i = 0; argv[2][i] >= '0' && argv[2][i] <= '9'; i++)
        {
            count = count * 10 + (argv[2][i] - '0');
        }
    }

    debug_dump_memory((uint32_t*)addr, count);
}

static void cmd_register_dump(uint32_t eax, uint32_t ebx, uint32_t ecx,
                              uint32_t edx, uint32_t esi, uint32_t edi,
                              uint32_t ebp, uint32_t esp, uint32_t eip)
{
    debug_dump_registers(eax, ebx, ecx, edx, esi, edi, ebp, esp, eip);
}

static void cmd_basic(void)
{
    basic_interactive_mode();
    console_write("\nExited BASIC interpreter\n");
}

static void cmd_spawn(void)
{
    console_write("Loading init.elf from initrd...\n");

    const void* elf_data = initrd_get_init();
    size_t elf_size = initrd_get_init_size();

    if (elf_size == 0)
    {
        console_write("Error: No init binary in initrd\n");
        return;
    }

    console_write("Init binary size: ");
    console_write_dec((int)elf_size);
    console_write(" bytes\n");

    struct elf_load_result result;
    page_directory_t* page_dir = vmm_get_current_directory();

    if (elf_load(elf_data, elf_size, page_dir, &result) != 0)
    {
        console_write("Error: Failed to load ELF binary\n");
        return;
    }

    console_write("Entry point: 0x");
    char hex[9];
    uint32_t entry = result.entry_point;
    for (int i = 7; i >= 0; i--)
    {
        uint8_t nibble = (entry >> (i * 4)) & 0xF;
        hex[7 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    hex[8] = '\0';
    console_write(hex);
    console_write("\n");

    struct task* t = task_create_user(result.entry_point, 1);
    if (t)
    {
        vterm_set_owner(VTERM_INIT, t->pid);
        log_info("User init spawned on terminal 1 (Alt+F2)");
        console_write("Created user task with PID ");
        console_write_dec(t->pid);
        console_write(" on terminal 1 (Alt+F2 to view)\n");
    }
    else
    {
        log_error("Failed to create user task");
        console_write("Failed to create user task\n");
    }
}

static void cmd_tty(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("Current terminal: ");
        console_write_dec(vterm_get_active_id());
        console_write("\n");
        console_write("Terminals:\n");
        for (int i = 0; i < VTERM_MAX_COUNT; i++)
        {
            struct vterm* vt = vterm_get(i);
            console_write("  ");
            console_write_dec(i);
            console_write(": ");
            console_write(vt->name);
            if (vt->owner_pid >= 0)
            {
                console_write(" (PID ");
                console_write_dec(vt->owner_pid);
                console_write(")");
            }
            if (vt->active)
            {
                console_write(" [active]");
            }
            console_write("\n");
        }
        console_write("Use Alt+F1-F4 to switch, or 'tty N'\n");
        return;
    }

    int term_id = argv[1][0] - '0';
    if (term_id >= 0 && term_id < VTERM_MAX_COUNT)
    {
        vterm_switch(term_id);
    }
    else
    {
        console_write("Invalid terminal ID (0-3)\n");
    }
}

static void fork_test_child(void)
{
    console_write("[child] Child process running\n");
    for (int i = 0; i < 3; i++)
    {
        console_write("[child] tick ");
        console_write_dec(i);
        console_write("\n");
        for (volatile int j = 0; j < 1000000; j++);
    }
    console_write("[child] Child exiting\n");
    struct task* t = sched_get_current();
    if (t)
    {
        task_exit(t->id, 0);
    }
    while (1) { hlt(); }
}

static void cmd_forktest(void)
{
    console_write("Creating fork test task...\n");
    struct task* t = task_create(fork_test_child, 1, true);
    if (t)
    {
        console_write("Created test task with PID ");
        console_write_dec(t->pid);
        console_write("\n");
    }
    else
    {
        console_write("Failed to create test task\n");
    }
}

static void cmd_dashboard(void)
{
    tui_init();
    tui_run_app();
}

static void cmd_test(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("Usage: test <command>\n");
        console_write("Commands:\n");
        console_write("  all           - Run all test suites\n");
        console_write("  list          - List available suites\n");
        console_write("  <suite>       - Run a specific suite\n");
        console_write("  <suite> <test>- Run a specific test\n");
        console_write("\nSuites: pmm, heap, string, fs, ipc, sched\n");
        return;
    }

    if (strcmp(argv[1], "all") == 0)
    {
        run_all_tests_console();
    }
    else if (strcmp(argv[1], "list") == 0)
    {
        console_write("Available test suites:\n");
        console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        console_write("  pmm    ");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("- Physical Memory Manager (8 tests)\n");
        console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        console_write("  heap   ");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("- Kernel Heap (12 tests)\n");
        console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        console_write("  string ");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("- String Functions (22 tests)\n");
        console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        console_write("  fs     ");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("- Filesystem (19 tests)\n");
        console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        console_write("  ipc    ");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("- Inter-Process Communication (11 tests)\n");
        console_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        console_write("  sched  ");
        console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        console_write("- Scheduler (11 tests)\n");
        console_write("\nTotal: 83 unit tests\n");
    }
    else if (argc == 2)
    {
        const struct test_suite* suite = test_get_suite_by_name(argv[1]);
        if (suite)
        {
            run_suite_console(argv[1]);
        }
        else
        {
            console_write("Unknown test suite: ");
            console_write(argv[1]);
            console_write("\nUse 'test list' to see available suites.\n");
        }
    }
    else
    {
        struct test_suite* suite = test_get_suite_by_name(argv[1]);
        if (!suite)
        {
            console_write("Unknown test suite: ");
            console_write(argv[1]);
            console_write("\n");
            return;
        }
        run_single_test_console(argv[1], argv[2]);
    }
}

static void cmd_unknown(const char* cmd)
{
    console_write("Unknown command: ");
    console_write(cmd);
    console_write("\nType 'help' for available commands.\n");
}

void execute_command(char* cmd)
{
    char* argv[MAX_ARGS];
    const int argc = parse_args(cmd, argv);

    if (argc == 0)
    {
        return;
    }

    if (strcmp(argv[0], "help") == 0)
    {
        cmd_help();
    }
    else if (strcmp(argv[0], "clear") == 0)
    {
        cmd_clear();
    }
    else if (strcmp(argv[0], "ps") == 0)
    {
        cmd_ps();
    }
    else if (strcmp(argv[0], "kill") == 0)
    {
        if (argc < 2)
        {
            console_write("kill: missing PID operand\n");
        }
        else
        {
            uint8_t pid = 0;
            for (size_t i = 0; i < strlen(argv[1]); i++)
            {
                if (argv[1][i] < '0' || argv[1][i] > '9')
                {
                    console_write("kill: invalid PID\n");
                    return;
                }
                pid = pid * 10 + (argv[1][i] - '0');
            }
            cmd_kill(pid);
        }
    }
    else if (strcmp(argv[0], "mem") == 0)
    {
        cmd_mem();
    }
    else if (strcmp(argv[0], "defrag") == 0)
    {
        cmd_defrag();
    }
    else if (strcmp(argv[0], "echo") == 0)
    {
        cmd_echo(argc, argv);
    }
    else if (strcmp(argv[0], "uptime") == 0)
    {
        cmd_uptime();
    }
    else if (strcmp(argv[0], "ver") == 0 || strcmp(argv[0], "version") == 0)
    {
        cmd_version();
    }
    else if (strcmp(argv[0], "ls") == 0)
    {
        cmd_ls(argc, argv);
    }
    else if (strcmp(argv[0], "cd") == 0)
    {
        cmd_cd(argc, argv);
    }
    else if (strcmp(argv[0], "pwd") == 0)
    {
        cmd_pwd();
    }
    else if (strcmp(argv[0], "cat") == 0)
    {
        cmd_cat(argc, argv);
    }
    else if (strcmp(argv[0], "mkdir") == 0)
    {
        cmd_mkdir(argc, argv);
    }
    else if (strcmp(argv[0], "rm") == 0)
    {
        cmd_rm(argc, argv);
    }
    else if (strcmp(argv[0], "rmdir") == 0)
    {
        cmd_rmdir(argc, argv);
    }
    else if (strcmp(argv[0], "touch") == 0)
    {
        cmd_touch(argc, argv);
    }
    else if (strcmp(argv[0], "edit") == 0)
    {
        cmd_edit(argc, argv);
    }
    else if (strcmp(argv[0], "write") == 0)
    {
        cmd_write(argc, argv);
    }
    else if (strcmp(argv[0], "log") == 0)
    {
        cmd_log();
    }
    else if (strcmp(argv[0], "clcache") == 0)
    {
        cmd_clear_cache();
    }
    else if (strcmp(argv[0], "shutdown") == 0)
    {
        cmd_shutdown();
    }
    else if (strcmp(argv[0], "reboot") == 0)
    {
        cmd_reboot();
    }
    else if (strcmp(argv[0], "cpu") == 0)
    {
        cmd_cpu();
    }
    else if (strcmp(argv[0], "sysmon") == 0)
    {
        cmd_sysmon();
    }
    else if (strcmp(argv[0], "trace") == 0)
    {
        cmd_trace();
    }
    else if (strcmp(argv[0], "clrtrace") == 0)
    {
        cmd_clear_trace();
    }
    else if (strcmp(argv[0], "memdump") == 0)
    {
        cmd_memdump(argc, argv);
    }
    else if (strcmp(argv[0], "registers") == 0)
    {
        uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip;
        arch_get_registers(&eax, &ebx, &ecx, &edx, &esi, &edi, &ebp, &esp, &eip);
        cmd_register_dump(eax, ebx, ecx, edx, esi, edi, ebp, esp, eip);
    }
    else if (strcmp(argv[0], "basic") == 0)
    {
        cmd_basic();
    }
    else if (strcmp(argv[0], "spawn") == 0)
    {
        cmd_spawn();
    }
    else if (strcmp(argv[0], "forktest") == 0)
    {
        cmd_forktest();
    }
    else if (strcmp(argv[0], "tty") == 0)
    {
        cmd_tty(argc, argv);
    }
    else if (strcmp(argv[0], "test") == 0)
    {
        cmd_test(argc, argv);
    }
    else if (strcmp(argv[0], "dash") == 0)
    {
        cmd_dashboard();
    }
    else
    {
        cmd_unknown(argv[0]);
    }
}

void shell_init(void)
{
    cmd_pos = 0;
    memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
    fs_init();
    sysmon_init();
    debug_utils_init();
    basic_init();
    editor_init();
    log_info("Filesystem initialized");
    log_info("System monitoring initialized");
    log_info("Debug utilities initialized");
    log_info("BASIC interpreter initialized");
    log_info("Editor initialized");
}

void shell_run(void)
{
    shell_init();
    log_info("Shell started");
    console_write("\nmexOS Shell - Type 'help' for commands\n\n");
    shell_prompt();

    history_pos = history_count;
    memset(temp_buffer, 0, CMD_BUFFER_SIZE);

    while (1)
    {
        const unsigned char c = keyboard_getchar();

        if (c == '\n')
        {
            console_putchar('\n');
            cmd_buffer[cmd_pos] = '\0';
            if (cmd_buffer[0] != '\0')
            {
                history_add(cmd_buffer);
            }
            execute_command(cmd_buffer);
            cmd_pos = 0;
            memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
            memset(temp_buffer, 0, CMD_BUFFER_SIZE);
            history_pos = history_count;
            shell_prompt();
        }
        else if (c == '\b')
        {
            if (cmd_pos > 0)
            {
                cmd_pos--;
                console_putchar('\b');
                console_putchar(' ');
                console_putchar('\b');
            }
        }
        else if (c == KEY_ARROW_UP)
        {
            if (history_count == 0)
            {
                continue;
            }

            if (history_pos == (int32_t)history_count)
            {
                strncpy(temp_buffer, cmd_buffer, CMD_BUFFER_SIZE);
            }

            const int32_t oldest = (int32_t)(history_count > HISTORY_SIZE ? history_count - HISTORY_SIZE : 0);

            if (history_pos > oldest)
            {
                history_pos--;
                clear_line();
                const uint32_t idx = (uint32_t)history_pos % HISTORY_SIZE;
                strncpy(cmd_buffer, history[idx], CMD_BUFFER_SIZE);
                cmd_buffer[CMD_BUFFER_SIZE - 1] = '\0';
                cmd_pos = strlen(cmd_buffer);
                display_buffer();

                timer_wait(2);
            }
        }
        else if (c == KEY_ARROW_DOWN)
        {
            if (history_count == 0 || history_pos >= (int32_t)history_count)
            {
                continue;
            }

            history_pos++;
            clear_line();

            if (history_pos >= (int32_t)history_count)
            {
                strncpy(cmd_buffer, temp_buffer, CMD_BUFFER_SIZE);
                cmd_buffer[CMD_BUFFER_SIZE - 1] = '\0';
            }
            else
            {
                const uint32_t idx = (uint32_t)history_pos % HISTORY_SIZE;
                strncpy(cmd_buffer, history[idx], CMD_BUFFER_SIZE);
                cmd_buffer[CMD_BUFFER_SIZE - 1] = '\0';
            }

            cmd_pos = strlen(cmd_buffer);
            display_buffer();

            timer_wait(2);
        }
        else if (c == KEY_HOME)
        {
            clear_line();
            cmd_pos = 0;
        }
        else if (c == KEY_END)
        {
            clear_line();
            cmd_pos = strlen(cmd_buffer);
            display_buffer();
        }
        else if (c >= 0x20 && c < 0x7F && cmd_pos < CMD_BUFFER_SIZE - 1)
        {
            cmd_buffer[cmd_pos++] = c;
            console_putchar(c);
        }
    }
}
