#include "runtime.h"

#define KCMD_MAX_LINE 256

static const char* basename_of(const char* path)
{
    const char* base = path;

    while (*path)
    {
        if (*path == '/')
        {
            base = path + 1;
        }
        path++;
    }

    return base;
}

int main(int argc, char** argv)
{
    if (argc <= 0 || !argv || !argv[0])
    {
        return 1;
    }

    char line[KCMD_MAX_LINE];
    size_t pos = 0;
    const char* command = basename_of(argv[0]);

    user_memset(line, 0, sizeof(line));

    const size_t command_len = user_strlen(command);
    if (command_len == 0 || command_len >= sizeof(line))
    {
        return 1;
    }

    user_memcpy(line + pos, command, command_len);
    pos += command_len;

    for (int i = 1; i < argc; i++)
    {
        const size_t arg_len = user_strlen(argv[i]);
        if (pos + 1 + arg_len >= sizeof(line))
        {
            user_println("kcmd: command line too long");
            return 1;
        }

        line[pos++] = ' ';
        user_memcpy(line + pos, argv[i], arg_len);
        pos += arg_len;
    }

    line[pos] = '\0';
    return shell_exec(line) < 0 ? 1 : 0;
}