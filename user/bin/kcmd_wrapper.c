#include "runtime.h"

#define KCMD_MAX_LINE 256

#ifndef KCMD_WRAPPER_NAME
    #define KCMD_WRAPPER_NAME ((const char*)0)
#endif

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

int main(const int argc, char** argv)
{
    char line[KCMD_MAX_LINE];
    size_t pos = 0;
    const char* command = KCMD_WRAPPER_NAME;

    if (!command && argc > 0 && argv && argv[0])
    {
        command = basename_of(argv[0]);
    }

    user_memset(line, 0, sizeof(line));

    if (!command)
    {
        user_println("kcmd: missing command name");
        return 1;
    }

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

    if (shell_exec(line) < 0)
    {
        user_print("kcmd: failed to execute ");
        user_println(command);
        return 1;
    }

    return 0;
}