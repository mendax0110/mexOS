#include "runtime.h"
#include "../../shared/user_abi.h"
#include "../shared/math.h"
#include "../shared/string_utils.h"

#define SH_BUFFER_SIZE 256
#define SH_MAX_ARGS 16
#define SH_HELP_MAX_PROGRAMS 64

/**
 * @brief Parse a command line into arguments
 * @param line The command line to parse
 * @param argv The array to store the parsed arguments
 * @return The number of arguments parsed
 */
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

/**
 * @brief Display the shell prompt with user and current directory information
 */
static void shell_prompt(void)
{
    struct user_info info;
    char cwd[128];

    if (getuser(&info) >= 0)
    {
        const char* path = user_get_directory_path();
        user_print(info.username);
        user_print("@mexOS:");
        user_print(path);
        user_print("$ ");
        return;
    }

    if (getcwd(cwd, sizeof(cwd)) >= 0)
    {
        user_print(cwd);
        user_print("$ ");
        return;
    }

    user_print("sh$ ");
}

/**
 * @brief Read a line of input from the user, handling backspace and line editing
 * @param buffer The buffer to store the input line
 * @param size The size of the buffer
 * @return The number of characters read, or -1 on error
 */
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

/**
 * @brief Resolve the full path of a program based on its name.
 * @param command The name of the program to resolve.
 * @param path The buffer to store the resolved path.
 * @param path_size The size of the path buffer.
 * @return 0 on success, -1 on failure.
 */
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
            copy_string(path, path_size, command);
            return 0;
        }

        return -1;
    }

    if (stat(command, &info) == 0 && info.type == FS_ABI_TYPE_FILE)
    {
        copy_string(path, path_size, command);
        return 0;
    }

    copy_string(path, path_size, "/bin/");
    const size_t cmd_len = user_strlen(command);
    if (cmd_len + 5 >= path_size)
    {
        return -1;
    }

    append_string(path, path_size, command);

    if (stat(path, &info) == 0 && info.type == FS_ABI_TYPE_FILE)
    {
        return 0;
    }

    return -1;
}

/**
 * @brief Run an external command, optionally in the background.
 * @param argc The number of arguments.
 * @param argv The array of argument strings.
 * @return The exit status of the command, or -1 on error.
 */
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

/**
 * @brief Execute a command that has already been resolved to a full path.
 * @param argv The array of argument strings.
 * @return The exit status of the command, or -1 on error.
 */
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

/**
 * @brief Run a pipeline of two commands.
 * @param left_line The command line for the left-hand side of the pipeline.
 * @param right_line The command line for the right-hand side of the pipeline.
 * @return The exit status of the pipeline, or 1 on error.
 */
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

/**
 * @brief Run a built-in command.
 * @param argc The number of arguments.
 * @param argv The array of argument strings.
 * @param handled A pointer to a boolean that will be set to true if the command is handled.
 * @return The exit status of the command, or -1 on error.
 */
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

/**
 * @brief The main entry point of the shell.
 * @param argc The argument count (unused).
 * @param argv The argument vector (unused).
 * @return Exit status code.
 */
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
