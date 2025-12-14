#include "shell.h"
#include "basic.h"
#include "editor.h"
#include "tui.h"
#include "../lib/ipc_client.h"
#include "../lib/memory.h"
#include "../../include/protocols/console_protocol.h"
#include "../../include/protocols/input_protocol.h"
#include "../../include/protocols/vfs_protocol.h"

/** Shell heap memory */
static uint8_t shell_heap[32768] __attribute__((aligned(4096)));

/** Command buffer */
#define CMD_BUFFER_SIZE 256
static char cmd_buffer[CMD_BUFFER_SIZE];
static uint32_t cmd_pos = 0;

/** Console server port */
static int console_port = -1;

/** Input server port */
static int input_port = -1;

/** VFS server port */
static int vfs_port = -1;

/**
 * @brief Write string to console
 * @param str String to write
 */
static void console_write(const char *str)
{
    struct message msg;
    struct console_write_request req;

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
}

/**
 * @brief Write character to console
 * @param c Character to write
 */
static void console_putchar(char c)
{
    char str[2] = { c, '\0' };
    console_write(str);
}

/**
 * @brief Clear the console
 */
static void console_clear(void)
{
    struct message msg;
    ipc_msg_init(&msg, CONSOLE_MSG_CLEAR);
    ipc_call(console_port, &msg);
}

/**
 * @brief Set console colors
 * @param fg Foreground color
 * @param bg Background color
 */
static void console_set_color(uint8_t fg, uint8_t bg)
{
    struct message msg;
    struct console_set_color_request req = { .foreground = fg, .background = bg };

    ipc_msg_init(&msg, CONSOLE_MSG_SET_COLOR);
    ipc_msg_set_data(&msg, &req, sizeof(req));
    ipc_call(console_port, &msg);
}

/**
 * @brief Read a character from input
 * @return Character read, or 0 if none available
 */
static char input_getchar(void)
{
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
 * @brief Display shell prompt
 */
static void shell_prompt(void)
{
    console_set_color(CONSOLE_COLOR_LIGHT_GREEN, CONSOLE_COLOR_BLACK);
    console_write("mexOS");
    console_set_color(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLACK);
    console_write("> ");
}

/**
 * @brief Compare strings
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal
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
 * @brief Execute a command
 * @param cmd Command string
 */
static void execute_command(const char *cmd)
{
    /* Skip leading whitespace */
    while (*cmd == ' ')
    {
        cmd++;
    }

    if (*cmd == '\0')
    {
        return;
    }

    if (str_cmp(cmd, "help") == 0)
    {
        console_write("Available commands:\n");
        console_write("  help     - Show this help\n");
        console_write("  clear    - Clear the screen\n");
        console_write("  ls       - List directory\n");
        console_write("  pwd      - Print working directory\n");
        console_write("  exit     - Exit shell\n");
        console_write("  version  - Show version\n");
    }
    else if (str_cmp(cmd, "clear") == 0)
    {
        console_clear();
    }
    else if (str_cmp(cmd, "version") == 0)
    {
        console_write("mexOS Microkernel v0.2\n");
        console_write("Shell running in user-space\n");
    }
    else if (str_cmp(cmd, "pwd") == 0)
    {
        console_write("/\n");
    }
    else if (str_cmp(cmd, "ls") == 0)
    {
        console_write("(empty directory)\n");
    }
    else if (str_cmp(cmd, "exit") == 0)
    {
        console_write("Goodbye!\n");
        /* TODO: Exit properly */
        while (1);
    }
    else
    {
        console_write("Unknown command: ");
        console_write(cmd);
        console_write("\n");
        console_write("Type 'help' for available commands.\n");
    }
}

/**
 * @brief Shell main function
 * @return Does not return
 */
int main(void)
{
    mem_init(shell_heap, sizeof(shell_heap));

    ipc_client_init();

    console_port = ipc_lookup_server("console");
    input_port = ipc_lookup_server("input");
    vfs_port = ipc_lookup_server("vfs");

    console_clear();
    console_set_color(CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_BLACK);
    console_write("========================================\n");
    console_write("     mexOS Microkernel Shell v0.2       \n");
    console_write("========================================\n");
    console_set_color(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLACK);
    console_write("\nType 'help' for available commands.\n\n");

    cmd_pos = 0;
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
            execute_command(cmd_buffer);
            cmd_pos = 0;
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
        else if (cmd_pos < CMD_BUFFER_SIZE - 1)
        {
            cmd_buffer[cmd_pos++] = c;
            console_putchar(c);
        }
    }

    return 0;
}