#include "../lib/ipc_client.h"
#include "../lib/memory.h"
#include "../../include/protocols/console_protocol.h"
#include "../../include/protocols/input_protocol.h"
#include "../../include/protocols/vfs_protocol.h"
#include "../../user/lib/syscall.h"
#include "../../kernel/include/types.h"

/** Shell heap memory */
static uint8_t shell_heap[65536] __attribute__((aligned(4096)));

/** Command buffer */
#define CMD_BUFFER_SIZE 256
#define MAX_ARGS 16
#define HISTORY_SIZE 32

static char cmd_buffer[CMD_BUFFER_SIZE];
static uint32_t cmd_pos = 0;

static char history[HISTORY_SIZE][CMD_BUFFER_SIZE];
static uint32_t history_count = 0;
static int32_t history_pos = 0;

/** Server ports */
static int console_port = -1;
static int input_port = -1;
static int vfs_port = -1;

/** Current working directory */
static char cwd[VFS_MAX_PATH] = "/";

/**
 * @brief String length
 */
static uint32_t str_len(const char *s)
{
    uint32_t len = 0;
    while (s[len]) len++;
    return len;
}

/**
 * @brief String copy
 */
static void str_copy(char *dst, const char *src, uint32_t max)
{
    uint32_t i = 0;
    while (src[i] && i < max - 1)
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/**
 * @brief String compare
 */
static int str_cmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/**
 * @brief Write string to console via IPC
 */
static void console_write(const char *str)
{
    if (console_port < 0 || !str) return;

    struct message msg;
    struct console_write_request req;

    while (*str)
    {
        ipc_msg_init(&msg, CONSOLE_MSG_WRITE);

        int len = 0;
        while (str[len] && len < CONSOLE_MAX_WRITE_SIZE)
        {
            len++;
        }

        req.length = (uint8_t)len;
        mem_copy(req.data, str, (uint32_t)len);
        ipc_msg_set_data(&msg, &req, sizeof(req));
        ipc_call(console_port, &msg);

        str += len;
    }
}

/**
 * @brief Write character to console
 */
static void console_putchar(char c)
{
    char str[2] = { c, '\0' };
    console_write(str);
}

/**
 * @brief Write decimal number to console
 */
static void console_write_dec(int32_t num)
{
    char buf[16];
    int i = 0;
    int neg = 0;

    if (num < 0)
    {
        neg = 1;
        num = -num;
    }

    if (num == 0)
    {
        console_putchar('0');
        return;
    }

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (neg) console_putchar('-');
    while (i > 0) console_putchar(buf[--i]);
}

/**
 * @brief Clear console
 */
static void console_clear(void)
{
    if (console_port < 0) return;

    struct message msg;
    ipc_msg_init(&msg, CONSOLE_MSG_CLEAR);
    ipc_call(console_port, &msg);
}

/**
 * @brief Set console color
 */
static void console_set_color(uint8_t fg, uint8_t bg)
{
    if (console_port < 0) return;

    struct message msg;
    struct console_set_color_request req = { .foreground = fg, .background = bg };

    ipc_msg_init(&msg, CONSOLE_MSG_SET_COLOR);
    ipc_msg_set_data(&msg, &req, sizeof(req));
    ipc_call(console_port, &msg);
}

/**
 * @brief Read character from input server
 */
static char input_getchar(void)
{
    if (input_port < 0) return 0;

    struct message msg;
    ipc_msg_init(&msg, INPUT_MSG_READ);

    if (ipc_call(input_port, &msg) == IPC_SUCCESS)
    {
        struct input_read_response resp;
        ipc_msg_get_data(&msg, &resp, sizeof(resp));

        if (resp.status == 0 && resp.event_count > 0)
        {
            if (resp.events[0].type == INPUT_EVENT_KEY_PRESS)
            {
                return (char)resp.events[0].keychar;
            }
        }
    }
    return 0;
}

/**
 * @brief VFS stat request
 */
static int vfs_stat(const char *path, struct vfs_stat *st)
{
    if (vfs_port < 0) return -1;

    struct message msg;
    struct vfs_stat_request req;
    struct vfs_stat_response resp;

    ipc_msg_init(&msg, VFS_MSG_STAT);
    str_copy(req.path, path, VFS_MAX_PATH);
    ipc_msg_set_data(&msg, &req, sizeof(req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        if (resp.status >= 0 && st)
        {
            *st = resp.info;
        }
        return resp.status;
    }
    return -1;
}

/**
 * @brief VFS readdir request
 */
static int vfs_readdir(const char *path)
{
    if (vfs_port < 0) return -1;

    struct message msg;
    struct vfs_path_request req;
    struct vfs_readdir_response resp;

    str_copy(req.path, path, VFS_MAX_PATH);

    int total = 0;
    int more = 1;

    while (more)
    {
        ipc_msg_init(&msg, VFS_MSG_READDIR);
        ipc_msg_set_data(&msg, &req, sizeof(req));

        if (ipc_call(vfs_port, &msg) != IPC_SUCCESS) return -1;

        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        if (resp.status < 0) return resp.status;

        for (int i = 0; i < resp.count; i++)
        {
            if (resp.entries[i].type == VFS_TYPE_DIR)
            {
                console_set_color(CONSOLE_COLOR_LIGHT_BLUE, CONSOLE_COLOR_BLACK);
            }
            else
            {
                console_set_color(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLACK);
            }
            console_write(resp.entries[i].name);
            console_write("\n");
            total++;
        }

        more = resp.more;
    }

    console_set_color(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLACK);
    return total;
}

/**
 * @brief VFS read file
 */
static int32_t vfs_read_file(const char *path, char *buffer, uint32_t size)
{
    if (vfs_port < 0) return -1;

    struct message msg;
    struct vfs_open_request open_req;
    struct vfs_open_response open_resp;

    ipc_msg_init(&msg, VFS_MSG_OPEN);
    open_req.flags = VFS_O_RDONLY;
    open_req.mode = 0;
    str_copy(open_req.path, path, VFS_MAX_PATH);
    ipc_msg_set_data(&msg, &open_req, sizeof(open_req));

    if (ipc_call(vfs_port, &msg) != IPC_SUCCESS) return -1;
    ipc_msg_get_data(&msg, &open_resp, sizeof(open_resp));
    if (open_resp.status < 0) return open_resp.status;

    int32_t fd = open_resp.fd;
    int32_t total = 0;
    uint32_t offset = 0;

    while (offset < size)
    {
        struct vfs_read_request read_req;
        struct vfs_read_response read_resp;

        ipc_msg_init(&msg, VFS_MSG_READ);
        read_req.fd = fd;
        read_req.size = (size - offset < VFS_MAX_DATA) ? (size - offset) : VFS_MAX_DATA;
        read_req.offset = offset;
        ipc_msg_set_data(&msg, &read_req, sizeof(read_req));

        if (ipc_call(vfs_port, &msg) != IPC_SUCCESS) break;
        ipc_msg_get_data(&msg, &read_resp, sizeof(read_resp));
        if (read_resp.status <= 0) break;

        mem_copy(buffer + offset, read_resp.data, (uint32_t)read_resp.status);
        offset += (uint32_t)read_resp.status;
        total += read_resp.status;

        if ((uint32_t)read_resp.status < read_req.size) break;
    }

    struct vfs_close_request close_req = { .fd = fd };
    ipc_msg_init(&msg, VFS_MSG_CLOSE);
    ipc_msg_set_data(&msg, &close_req, sizeof(close_req));
    ipc_call(vfs_port, &msg);

    return total;
}

/**
 * @brief VFS write file
 */
static int32_t vfs_write_file(const char *path, const char *data, uint32_t size)
{
    if (vfs_port < 0) return -1;

    struct message msg;
    struct vfs_open_request open_req;
    struct vfs_open_response open_resp;

    ipc_msg_init(&msg, VFS_MSG_OPEN);
    open_req.flags = VFS_O_WRONLY | VFS_O_CREATE | VFS_O_TRUNC;
    open_req.mode = 0;
    str_copy(open_req.path, path, VFS_MAX_PATH);
    ipc_msg_set_data(&msg, &open_req, sizeof(open_req));

    if (ipc_call(vfs_port, &msg) != IPC_SUCCESS) return -1;
    ipc_msg_get_data(&msg, &open_resp, sizeof(open_resp));
    if (open_resp.status < 0) return open_resp.status;

    int32_t fd = open_resp.fd;
    int32_t total = 0;
    uint32_t offset = 0;

    while (offset < size)
    {
        struct vfs_write_request write_req;
        struct vfs_write_response write_resp;

        ipc_msg_init(&msg, VFS_MSG_WRITE);
        write_req.fd = fd;
        write_req.size = (size - offset < VFS_MAX_DATA) ? (size - offset) : VFS_MAX_DATA;
        mem_copy(write_req.data, data + offset, write_req.size);
        ipc_msg_set_data(&msg, &write_req, sizeof(write_req));

        if (ipc_call(vfs_port, &msg) != IPC_SUCCESS) break;
        ipc_msg_get_data(&msg, &write_resp, sizeof(write_resp));
        if (write_resp.status < 0) break;

        offset += write_req.size;
        total += (int32_t)write_req.size;
    }

    struct vfs_close_request close_req = { .fd = fd };
    ipc_msg_init(&msg, VFS_MSG_CLOSE);
    ipc_msg_set_data(&msg, &close_req, sizeof(close_req));
    ipc_call(vfs_port, &msg);

    return total;
}

/**
 * @brief VFS mkdir
 */
static int vfs_mkdir(const char *path)
{
    if (vfs_port < 0) return -1;

    struct message msg;
    struct vfs_path_request req;
    struct vfs_response resp;

    ipc_msg_init(&msg, VFS_MSG_MKDIR);
    str_copy(req.path, path, VFS_MAX_PATH);
    ipc_msg_set_data(&msg, &req, sizeof(req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        return resp.status;
    }
    return -1;
}

/**
 * @brief VFS unlink (remove file/dir)
 */
static int vfs_unlink(const char *path)
{
    if (vfs_port < 0) return -1;

    struct message msg;
    struct vfs_path_request req;
    struct vfs_response resp;

    ipc_msg_init(&msg, VFS_MSG_UNLINK);
    str_copy(req.path, path, VFS_MAX_PATH);
    ipc_msg_set_data(&msg, &req, sizeof(req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        return resp.status;
    }
    return -1;
}

/**
 * @brief VFS chdir
 */
static int vfs_chdir(const char *path)
{
    if (vfs_port < 0) return -1;

    struct message msg;
    struct vfs_path_request req;
    struct vfs_response resp;

    ipc_msg_init(&msg, VFS_MSG_CHDIR);
    str_copy(req.path, path, VFS_MAX_PATH);
    ipc_msg_set_data(&msg, &req, sizeof(req));

    if (ipc_call(vfs_port, &msg) == IPC_SUCCESS)
    {
        ipc_msg_get_data(&msg, &resp, sizeof(resp));
        if (resp.status >= 0)
        {
            str_copy(cwd, path, VFS_MAX_PATH);
        }
        return resp.status;
    }
    return -1;
}

/**
 * @brief Display shell prompt
 */
static void shell_prompt(void)
{
    console_set_color(CONSOLE_COLOR_LIGHT_GREEN, CONSOLE_COLOR_BLACK);
    console_write("mexOS");
    console_set_color(CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_BLACK);
    console_write(":");
    console_write(cwd);
    console_set_color(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLACK);
    console_write("$ ");
}

/**
 * @brief Parse command arguments
 */
static int parse_args(char *cmd, char *argv[])
{
    int argc = 0;
    char *p = cmd;

    while (*p && argc < MAX_ARGS)
    {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return argc;
}

/**
 * @brief Command: help
 */
static void cmd_help(void)
{
    console_write("Available commands:\n");
    console_write("  help       - Show this help\n");
    console_write("  clear      - Clear screen\n");
    console_write("  ls [path]  - List directory\n");
    console_write("  cd <path>  - Change directory\n");
    console_write("  pwd        - Print working directory\n");
    console_write("  cat <file> - Display file contents\n");
    console_write("  mkdir <dir>- Create directory\n");
    console_write("  rm <path>  - Remove file or directory\n");
    console_write("  touch <f>  - Create empty file\n");
    console_write("  write <f> <text> - Write text to file\n");
    console_write("  echo <...> - Echo arguments\n");
    console_write("  uptime     - Show system uptime\n");
    console_write("  version    - Show version\n");
    console_write("  exit       - Exit shell\n");
}

/**
 * @brief Command: ls
 */
static void cmd_ls(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : cwd;
    int ret = vfs_readdir(path);
    if (ret < 0)
    {
        console_write("ls: cannot access '");
        console_write(path);
        console_write("'\n");
    }
    else if (ret == 0)
    {
        console_write("(empty)\n");
    }
}

/**
 * @brief Command: cd
 */
static void cmd_cd(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "/";
    if (vfs_chdir(path) < 0)
    {
        console_write("cd: no such directory '");
        console_write(path);
        console_write("'\n");
    }
}

/**
 * @brief Command: pwd
 */
static void cmd_pwd(void)
{
    console_write(cwd);
    console_write("\n");
}

/**
 * @brief Command: cat
 */
static void cmd_cat(int argc, char *argv[])
{
    if (argc < 2)
    {
        console_write("cat: missing file\n");
        return;
    }

    static char file_buf[4096];
    int32_t ret = vfs_read_file(argv[1], file_buf, sizeof(file_buf) - 1);

    if (ret < 0)
    {
        console_write("cat: cannot read '");
        console_write(argv[1]);
        console_write("'\n");
        return;
    }

    file_buf[ret] = '\0';
    console_write(file_buf);
    if (ret > 0 && file_buf[ret - 1] != '\n')
    {
        console_write("\n");
    }
}

/**
 * @brief Command: mkdir
 */
static void cmd_mkdir(int argc, char *argv[])
{
    if (argc < 2)
    {
        console_write("mkdir: missing directory name\n");
        return;
    }

    if (vfs_mkdir(argv[1]) < 0)
    {
        console_write("mkdir: cannot create '");
        console_write(argv[1]);
        console_write("'\n");
    }
}

/**
 * @brief Command: rm
 */
static void cmd_rm(int argc, char *argv[])
{
    if (argc < 2)
    {
        console_write("rm: missing operand\n");
        return;
    }

    if (vfs_unlink(argv[1]) < 0)
    {
        console_write("rm: cannot remove '");
        console_write(argv[1]);
        console_write("'\n");
    }
}

/**
 * @brief Command: touch
 */
static void cmd_touch(int argc, char *argv[])
{
    if (argc < 2)
    {
        console_write("touch: missing file\n");
        return;
    }

    if (vfs_write_file(argv[1], "", 0) < 0)
    {
        console_write("touch: cannot create '");
        console_write(argv[1]);
        console_write("'\n");
    }
}

/**
 * @brief Command: write
 */
static void cmd_write(int argc, char *argv[])
{
    if (argc < 3)
    {
        console_write("write: usage: write <file> <text>\n");
        return;
    }

    static char content[4096];
    uint32_t pos = 0;

    for (int i = 2; i < argc && pos < sizeof(content) - 2; i++)
    {
        if (i > 2) content[pos++] = ' ';
        uint32_t len = str_len(argv[i]);
        if (pos + len >= sizeof(content) - 1) break;
        mem_copy(content + pos, argv[i], len);
        pos += len;
    }
    content[pos++] = '\n';
    content[pos] = '\0';

    if (vfs_write_file(argv[1], content, pos) < 0)
    {
        console_write("write: cannot write '");
        console_write(argv[1]);
        console_write("'\n");
    }
}

/**
 * @brief Command: echo
 */
static void cmd_echo(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        console_write(argv[i]);
        if (i < argc - 1) console_putchar(' ');
    }
    console_putchar('\n');
}

/**
 * @brief Command: uptime
 */
static void cmd_uptime(void)
{
    uint32_t ticks = sys_get_ticks();
    uint32_t seconds = ticks / 100;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    console_write("Uptime: ");
    console_write_dec((int32_t)hours);
    console_write("h ");
    console_write_dec((int32_t)(minutes % 60));
    console_write("m ");
    console_write_dec((int32_t)(seconds % 60));
    console_write("s\n");
}

/**
 * @brief Command: version
 */
static void cmd_version(void)
{
    console_write("mexOS Microkernel v0.2\n");
    console_write("Architecture: i686\n");
    console_write("Shell running in user-space via IPC\n");
}

/**
 * @brief Clear command line
 */
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

/**
 * @brief Display command buffer
 */
static void display_buffer(void)
{
    for (uint32_t i = 0; i < cmd_pos; i++)
    {
        console_putchar(cmd_buffer[i]);
    }
}

/**
 * @brief Add command to history
 */
static void history_add(const char *cmd)
{
    if (cmd[0] == '\0') return;

    if (history_count > 0 &&
        str_cmp(history[(history_count - 1) % HISTORY_SIZE], cmd) == 0)
    {
        return;
    }

    str_copy(history[history_count % HISTORY_SIZE], cmd, CMD_BUFFER_SIZE);
    history_count++;
}

/**
 * @brief Execute command
 */
static void execute_command(char *cmd)
{
    char *argv[MAX_ARGS];
    int argc = parse_args(cmd, argv);

    if (argc == 0) return;

    if (str_cmp(argv[0], "help") == 0)
    {
        cmd_help();
    }
    else if (str_cmp(argv[0], "clear") == 0)
    {
        console_clear();
    }
    else if (str_cmp(argv[0], "ls") == 0)
    {
        cmd_ls(argc, argv);
    }
    else if (str_cmp(argv[0], "cd") == 0)
    {
        cmd_cd(argc, argv);
    }
    else if (str_cmp(argv[0], "pwd") == 0)
    {
        cmd_pwd();
    }
    else if (str_cmp(argv[0], "cat") == 0)
    {
        cmd_cat(argc, argv);
    }
    else if (str_cmp(argv[0], "mkdir") == 0)
    {
        cmd_mkdir(argc, argv);
    }
    else if (str_cmp(argv[0], "rm") == 0 || str_cmp(argv[0], "rmdir") == 0)
    {
        cmd_rm(argc, argv);
    }
    else if (str_cmp(argv[0], "touch") == 0)
    {
        cmd_touch(argc, argv);
    }
    else if (str_cmp(argv[0], "write") == 0)
    {
        cmd_write(argc, argv);
    }
    else if (str_cmp(argv[0], "echo") == 0)
    {
        cmd_echo(argc, argv);
    }
    else if (str_cmp(argv[0], "uptime") == 0)
    {
        cmd_uptime();
    }
    else if (str_cmp(argv[0], "ver") == 0 || str_cmp(argv[0], "version") == 0)
    {
        cmd_version();
    }
    else if (str_cmp(argv[0], "exit") == 0)
    {
        console_write("Goodbye!\n");
        sys_exit(0);
    }
    else
    {
        console_write("Unknown command: ");
        console_write(argv[0]);
        console_write("\nType 'help' for available commands.\n");
    }
}

/**
 * @brief Shell main
 */
int main(void)
{
    mem_init(shell_heap, sizeof(shell_heap));
    ipc_client_init();

    console_port = ipc_lookup_server(CONSOLE_SERVER_PORT_NAME);
    input_port = ipc_lookup_server(INPUT_SERVER_PORT_NAME);
    vfs_port = ipc_lookup_server(VFS_SERVER_PORT_NAME);

    console_clear();
    console_set_color(CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_BLACK);
    console_write("========================================\n");
    console_write("      mexOS Microkernel Shell v0.2      \n");
    console_write("========================================\n");
    console_set_color(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLACK);
    console_write("\nType 'help' for available commands.\n\n");

    cmd_pos = 0;
    history_pos = 0;
    shell_prompt();

    while (1)
    {
        char c = input_getchar();

        if (c == 0)
        {
            for (volatile int i = 0; i < 10000; i++);
            continue;
        }

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
            history_pos = (int32_t)history_count;
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
        else if ((uint8_t)c == 0x80) /* Arrow Up */
        {
            if (history_count > 0 && history_pos > 0)
            {
                history_pos--;
                clear_line();
                str_copy(cmd_buffer, history[history_pos % HISTORY_SIZE], CMD_BUFFER_SIZE);
                cmd_pos = str_len(cmd_buffer);
                display_buffer();
            }
        }
        else if ((uint8_t)c == 0x81) /* Arrow Down */
        {
            if (history_pos < (int32_t)history_count - 1)
            {
                history_pos++;
                clear_line();
                str_copy(cmd_buffer, history[history_pos % HISTORY_SIZE], CMD_BUFFER_SIZE);
                cmd_pos = str_len(cmd_buffer);
                display_buffer();
            }
            else if (history_pos == (int32_t)history_count - 1)
            {
                history_pos++;
                clear_line();
                cmd_pos = 0;
            }
        }
        else if (c >= 0x20 && c < 0x7F && cmd_pos < CMD_BUFFER_SIZE - 1)
        {
            cmd_buffer[cmd_pos++] = c;
            console_putchar(c);
        }
    }

    return 0;
}