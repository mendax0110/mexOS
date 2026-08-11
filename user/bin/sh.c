#include "runtime.h"
#include "../shared/math.h"
#include "../shared/string_utils.h"

#define SH_BUFFER_SIZE 256
#define SH_MAX_ARGS 16
#define SH_HELP_MAX_PROGRAMS 64

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

static int run_external(const int argc, char* argv[])
{
    const bool background = argc > 1 && user_streq(argv[argc - 1], "&");
    if (background) argv[argc - 1] = NULL;
    char path[128];
    if (resolve_program_path(argv[0], path, sizeof(path)) != 0)
    {
        return -1;
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

    if (background)
    {
        setpgid(child, child);
        user_print("[");
        user_print_dec(child);
        user_println("]");
        return 0;
    }

    int status = 0;
    if (wait(child, &status) < 0)
    {
        user_println("sh: wait failed");
        return 1;
    }

    UNUSED(argc);
    return status;
}

static int exec_resolved(char* argv[])
{
    char path[128];
    if (resolve_program_path(argv[0], path, sizeof(path)) != 0)
    {
        user_print("sh: command not found: ");
        user_println(argv[0]);
        return -1;
    }
    execv(path, (const char* const*)argv);
    return -1;
}

static int run_pipeline(char* left_line, char* right_line)
{
    char* left_args[SH_MAX_ARGS + 1];
    char* right_args[SH_MAX_ARGS + 1];
    if (parse_args(left_line, left_args) == 0 || parse_args(right_line, right_args) == 0)
    {
        return 1;
    }

    int fds[2];
    if (pipe(fds) < 0)
    {
        user_println("sh: pipe failed");
        return 1;
    }

    const int left = fork();
    if (left == 0)
    {
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        exec_resolved(left_args);
        exit(127);
    }

    const int right = fork();
    if (right == 0)
    {
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);
        exec_resolved(right_args);
        exit(127);
    }

    close(fds[0]);
    close(fds[1]);
    if (left < 0 || right < 0) return 1;
    setpgid(left, left);
    setpgid(right, left);
    int status = 0;
    wait(left, &status);
    wait(right, &status);
    return status;
}

static int run_builtin(const int argc, char* argv[], bool* handled)
{
    *handled = true;

    if (user_streq(argv[0], "exit"))
    {
        return argc > 1 ? parse_int(argv[1]) : 0;
    }

    if (user_streq(argv[0], "cd"))
    {
        if (chdir(argc > 1 ? argv[1] : "/") < 0)
        {
            user_println("sh: cd failed");
            return 1;
        }
        return 0;
    }

    if (user_streq(argv[0], "help"))
    {
        user_println("Built-ins: cd exit help");
        struct fs_dirent programs[SH_HELP_MAX_PROGRAMS];
        const int count = readdir("/bin", programs, SH_HELP_MAX_PROGRAMS);
        if (count < 0)
        {
            user_println("Programs: unable to read /bin");
            return 1;
        }

        user_println("Installed programs:");
        for (int i = 0; i < count; i++)
        {
            if (programs[i].type != FS_ABI_TYPE_FILE)
            {
                continue;
            }
            user_print("  ");
            user_println(programs[i].name);
        }
        return 0;
    }

    *handled = false;
    return -1;
}

int main(const int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    user_println("[sh] mexOS user shell");

    char line[SH_BUFFER_SIZE];
    char* args[SH_MAX_ARGS + 1];

    while (1)
    {
        int child_status = 0;
        while (waitpid(-1, &child_status, WAIT_NOHANG) > 0)
        {
        }
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

        char* pipeline = NULL;
        for (int i = 0; i < line_len; i++)
        {
            if (line[i] == '|')
            {
                pipeline = &line[i];
                break;
            }
        }
        if (pipeline)
        {
            *pipeline = '\0';
            run_pipeline(line, pipeline + 1);
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

        if (run_external(arg_count, args) >= 0)
        {
            continue;
        }

        user_print("sh: command not found: ");
        user_println(args[0]);
    }
}
