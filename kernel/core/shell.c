#include "shell.h"
#include "console.h"
#include "keyboard.h"
#include "fs.h"
#include "log.h"
#include "../include/string.h"
#include "../sched/sched.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../arch/i686/arch.h"
#include "timer.h"

#define CMD_BUFFER_SIZE 256
#define MAX_ARGS 16
#define EDITOR_MAX_LINES 64
#define EDITOR_LINE_SIZE 80

static char cmd_buffer[CMD_BUFFER_SIZE];
static uint32_t cmd_pos = 0;

static char editor_buf[FS_MAX_FILE_SIZE];
static char editor_line_buf[EDITOR_LINE_SIZE];

static void shell_prompt(void)
{
    console_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    console_write("mexOS");
    console_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    console_write("> ");
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
    console_write("  mem     - Show memory usage\n");
    console_write("  echo    - Echo arguments\n");
    console_write("  uptime  - Show system uptime\n");
    console_write("  ver     - Show version info\n");
    console_write("  ls      - List directory contents\n");
    console_write("  cd      - Change directory\n");
    console_write("  pwd     - Print working directory\n");
    console_write("  cat     - Display file contents\n");
    console_write("  mkdir   - Create a new directory\n");
    console_write("  rm      - Remove a file or directory\n");
    console_write("  touch   - Create an empty file\n");
    console_write("  edit    - Edit a file\n");
    console_write("  write   - Write text to file\n");
    console_write("  log     - Show system log\n");
    console_write("  clcache - Clear filesystem cache\n");
    console_write("  shutdown- Shutdown the system\n");
    console_write("  reboot  - Reboot the system\n");
}

static void cmd_clear(void)
{
    console_clear();
}

static void cmd_ps(void)
{
    struct task* current = sched_get_current();
    console_write("PID  STATE    PRIORITY\n");
    console_write("----------------------\n");
    if (current)
    {
        console_write("  ");
        console_write_dec(current->pid);
        console_write("  RUNNING  ");
        console_write_dec(current->priority);
        console_write("\n");
    }
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

    console_write("Kernel Heap:\n");
    console_write("  Used: ");
    console_write_dec(heap_get_used());
    console_write(" bytes\n  Free: ");
    console_write_dec(heap_get_free());
    console_write(" bytes\n");
}

static void cmd_echo(int argc, char* argv[])
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
    uint32_t ticks = timer_get_ticks();
    uint32_t seconds = ticks / 100;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

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

static void cmd_ls(int argc, char* argv[])
{
    const char* path = (argc > 1) ? argv[1] : ".";
    char buffer[1024];

    int ret = fs_list_dir(path, buffer, sizeof(buffer));
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

static void cmd_cd(int argc, char* argv[])
{
    const char* path = (argc > 1) ? argv[1] : "/";

    int ret = fs_change_dir(path);
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

static void cmd_cat(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("cat: missing file operand\n");
        return;
    }

    char buffer[FS_MAX_FILE_SIZE + 1];
    int ret = fs_read(argv[1], buffer, FS_MAX_FILE_SIZE);

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

static void cmd_mkdir(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("mkdir: missing directory name\n");
        return;
    }

    int ret = fs_create_dir(argv[1]);
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

static void cmd_rm(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("rm: missing operand\n");
        return;
    }

    int ret = fs_remove(argv[1]);
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

static void cmd_touch(int argc, char* argv[])
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

    int ret = fs_create_file(argv[1]);
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

static void cmd_edit(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_write("edit: missing file operand\n");
        return;
    }

    const char* filename = argv[1];

    if (!fs_exists(filename))
    {
        int ret = fs_create_file(filename);
        if (ret != FS_ERR_OK)
        {
            console_write("edit: cannot create file\n");
            return;
        }
    }
    else if (fs_is_dir(filename))
    {
        console_write("edit: is a directory\n");
        return;
    }

    int file_len = fs_read(filename, editor_buf, FS_MAX_FILE_SIZE - 1);
    if (file_len < 0)
    {
        file_len = 0;
    }
    editor_buf[file_len] = '\0';

    console_clear();
    console_write("=== Editor: ");
    console_write(filename);
    console_write(" ===\n");
    console_write(":q quit | :w save | :wq save+quit | :d delete line\n");
    console_write("---\n");

    if (file_len > 0)
    {
        console_write(editor_buf);
        if (editor_buf[file_len - 1] != '\n')
        {
            console_write("\n");
        }
    }
    console_write("---\n> ");

    int running = 1;
    while (running)
    {
        uint32_t pos = 0;
        memset(editor_line_buf, 0, EDITOR_LINE_SIZE);

        while (1)
        {
            char c = keyboard_getchar();

            if (c == '\n')
            {
                console_putchar('\n');
                editor_line_buf[pos] = '\0';
                break;
            }
            else if (c == '\b')
            {
                if (pos > 0)
                {
                    pos--;
                    console_putchar('\b');
                    console_putchar(' ');
                    console_putchar('\b');
                }
            }
            else if (c >= 0x20 && c < 0x7F && pos < EDITOR_LINE_SIZE - 1)
            {
                editor_line_buf[pos++] = c;
                console_putchar(c);
            }
        }

        if (editor_line_buf[0] == ':')
        {
            if (strcmp(editor_line_buf, ":q") == 0)
            {
                running = 0;
            }
            else if (strcmp(editor_line_buf, ":w") == 0)
            {
                fs_write(filename, editor_buf, (uint32_t)strlen(editor_buf));
                console_write("Saved\n> ");
            }
            else if (strcmp(editor_line_buf, ":wq") == 0)
            {
                fs_write(filename, editor_buf, (uint32_t)strlen(editor_buf));
                console_write("Saved\n");
                running = 0;
            }
            else if (strcmp(editor_line_buf, ":d") == 0)
            {
                uint32_t len = (uint32_t)strlen(editor_buf);
                if (len > 0)
                {
                    uint32_t last_nl = len;
                    if (editor_buf[len - 1] == '\n' && len > 1)
                    {
                        last_nl = len - 1;
                    }
                    while (last_nl > 0 && editor_buf[last_nl - 1] != '\n')
                    {
                        last_nl--;
                    }
                    editor_buf[last_nl] = '\0';
                    console_write("Line deleted\n> ");
                }
                else
                {
                    console_write("Buffer empty\n> ");
                }
            }
            else if (strcmp(editor_line_buf, ":p") == 0)
            {
                console_write("---\n");
                if (strlen(editor_buf) > 0)
                {
                    console_write(editor_buf);
                    if (editor_buf[strlen(editor_buf) - 1] != '\n')
                    {
                        console_write("\n");
                    }
                }
                console_write("---\n> ");
            }
            else
            {
                console_write("Unknown command\n> ");
            }
        }
        else
        {
            uint32_t buf_len = (uint32_t)strlen(editor_buf);
            uint32_t line_len = (uint32_t)strlen(editor_line_buf);

            if (buf_len + line_len + 1 < FS_MAX_FILE_SIZE)
            {
                strcat(editor_buf, editor_line_buf);
                strcat(editor_buf, "\n");
                console_write("> ");
            }
            else
            {
                console_write("Buffer full\n> ");
            }
        }
    }

    console_clear();
}

static void cmd_write(int argc, char* argv[])
{
    if (argc < 3)
    {
        console_write("write: usage: write <file> <text>\n");
        return;
    }

    const char* filename = argv[1];

    if (!fs_exists(filename))
    {
        int ret = fs_create_file(filename);
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
        uint32_t len = (uint32_t)strlen(argv[i]);
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

static void cmd_unknown(const char* cmd)
{
    console_write("Unknown command: ");
    console_write(cmd);
    console_write("\nType 'help' for available commands.\n");
}

static void execute_command(char* cmd)
{
    char* argv[MAX_ARGS];
    int argc = parse_args(cmd, argv);

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
    else if (strcmp(argv[0], "mem") == 0)
    {
        cmd_mem();
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
    log_info("Filesystem initialized");
}

void shell_run(void)
{
    shell_init();
    log_info("Shell started");
    console_write("\nmexOS Shell - Type 'help' for commands\n\n");
    shell_prompt();

    while (1)
    {
        char c = keyboard_getchar();

        if (c == '\n')
        {
            console_putchar('\n');
            cmd_buffer[cmd_pos] = '\0';
            execute_command(cmd_buffer);
            cmd_pos = 0;
            memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
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
        else if (c >= 0x20 && c < 0x7F && cmd_pos < CMD_BUFFER_SIZE - 1)
        {
            cmd_buffer[cmd_pos++] = c;
            console_putchar(c);
        }
    }
}
