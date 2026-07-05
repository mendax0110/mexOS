#include "runtime.h"

#define SH_BUFFER_SIZE 256
#define SH_MAX_ARGS 16

static int parse_args(char* line, char* argv[])
{
    int argc = 0;
    char* p = line;

    while (*p && argc < SH_MAX_ARGS)
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

    argv[argc] = NULL;
    return argc;
}

static int parse_int(const char* str)
{
    int value = 0;
    int sign = 1;

    if (*str == '-')
    {
        sign = -1;
        str++;
    }

    while (*str >= '0' && *str <= '9')
    {
        value = (value * 10) + (*str - '0');
        str++;
    }

    return value * sign;
}

static void shell_prompt(void)
{
    char cwd[128];
    if (getcwd(cwd, sizeof(cwd)) >= 0)
    {
        user_print(cwd);
        user_print(" $ ");
        return;
    }

    user_print("sh$ ");
}

static int shell_read_line(char* buffer, const size_t size)
{
    size_t pos = 0;

    while (1)
    {
        char ch = 0;
        const int result = read(STDIN_FILENO, &ch, 1);
        if (result < 0)
        {
            return -1;
        }

        if (result == 0)
        {
            continue;
        }

        if (ch == '\r')
        {
            continue;
        }

        if (ch == '\n')
        {
            user_putc('\n');
            buffer[pos] = '\0';
            return (int)pos;
        }

        if (ch == '\b')
        {
            if (pos > 0)
            {
                pos--;
                user_print("\b \b");
            }
            continue;
        }

        if (ch < 0x20 || ch >= 0x7F || pos + 1 >= size)
        {
            continue;
        }

        buffer[pos++] = ch;
        user_putc(ch);
    }
}

static int resolve_program_path(const char* command, char* path, const size_t path_size)
{
    if (!command || !path || path_size == 0)
    {
        return -1;
    }

    struct fs_stat info;
    if (user_has_slash(command))
    {
        if (stat(command, &info) == 0 && info.type == FS_ABI_TYPE_FILE)
        {
            user_memset(path, 0, path_size);
            user_memcpy(path, command, user_strlen(command) < path_size - 1 ? user_strlen(command) : path_size - 1);
            return 0;
        }

        return -1;
    }

    if (stat(command, &info) == 0 && info.type == FS_ABI_TYPE_FILE)
    {
        user_memset(path, 0, path_size);
        user_memcpy(path, command, user_strlen(command) < path_size - 1 ? user_strlen(command) : path_size - 1);
        return 0;
    }

    user_memset(path, 0, path_size);
    user_memcpy(path, "/bin/", 5);
    const size_t cmd_len = user_strlen(command);
    if (5 + cmd_len >= path_size)
    {
        return -1;
    }

    user_memcpy(path + 5, command, cmd_len);
    path[5 + cmd_len] = '\0';

    if (stat(path, &info) == 0 && info.type == FS_ABI_TYPE_FILE)
    {
        return 0;
    }

    return -1;
}

static int run_external(int argc, char* argv[])
{
    char path[128];
    if (resolve_program_path(argv[0], path, sizeof(path)) != 0)
    {
        user_print("sh: command not found: ");
        user_println(argv[0]);
        return 127;
    }

    const int child = fork();
    if (child < 0)
    {
        user_println("sh: fork failed");
        return 1;
    }

    if (child == 0)
    {
        if (execv(path, (const char* const*)argv) < 0)
        {
            user_print("sh: failed to exec ");
            user_println(path);
            exit(127);
        }

        exit(127);
    }

    int status = 0;
    if (wait(child, &status) < 0)
    {
        user_println("sh: wait failed");
        return 1;
    }

    (void)argc;
    return status;
}

static int run_builtin(int argc, char* argv[], bool* handled)
{
    *handled = true;

    if (user_streq(argv[0], "exit"))
    {
        return argc > 1 ? parse_int(argv[1]) : 0;
    }

    if (user_streq(argv[0], "cd"))
    {
        const char* path = (argc > 1) ? argv[1] : "/";
        if (chdir(path) != 0)
        {
            user_print("sh: cd: ");
            user_println(path);
        }
        return -1;
    }

    if (user_streq(argv[0], "pwd"))
    {
        char cwd[128];
        if (getcwd(cwd, sizeof(cwd)) >= 0)
        {
            user_println(cwd);
        }
        else
        {
            user_println("sh: pwd failed");
        }
        return -1;
    }

    if (user_streq(argv[0], "help"))
    {
        user_println("Builtins: help cd pwd exit");
        user_println("Programs: /bin/echo /bin/ls /bin/cat /bin/sh");
        return -1;
    }

    *handled = false;
    return -1;
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    user_println("[sh] mexOS user shell");

    char line[SH_BUFFER_SIZE];
    char* args[SH_MAX_ARGS + 1];

    while (1)
    {
        shell_prompt();

        const int line_len = shell_read_line(line, sizeof(line));
        if (line_len < 0)
        {
            user_println("sh: input error");
            return 1;
        }

        if (line_len == 0)
        {
            continue;
        }

        const int arg_count = parse_args(line, args);
        if (arg_count == 0)
        {
            continue;
        }

        bool handled = false;
        const int builtin_rc = run_builtin(arg_count, args, &handled);
        if (handled)
        {
            if (user_streq(args[0], "exit"))
            {
                return builtin_rc;
            }
            continue;
        }

        run_external(arg_count, args);
    }
}
