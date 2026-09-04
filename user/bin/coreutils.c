#include "runtime.h"
#include "../shared/math.h"
#include "../shared/string_utils.h"

#ifndef USER_APP_NAME
#define USER_APP_NAME ((const char*)0)
#endif

/**
 * @brief Get the application name from the command line arguments.
 * @param argc The number of command line arguments
 * @param argv The array of command line argument strings
 * @return The application name as a string
 */
static const char* app_name(const int argc, char** argv)
{
    if (USER_APP_NAME) return USER_APP_NAME;
    if (argc <= 0 || !argv || !argv[0]) return "";
    const char* base = argv[0];
    for (const char* p = argv[0]; *p; p++)
    {
        if (*p == '/') base = p + 1;
    }
    return base;
}

/**
 * @brief Print the list of processes in a formatted table.
 */
static void print_processes(void)
{
    struct process_info processes[32];
    const int count = getprocs(processes, 32);
    user_println(" PID  PPID UID STATE CPU");
    for (int i = 0; i < count; i++)
    {
        user_print_dec(processes[i].pid);
        user_print("    ");
        user_print_dec(processes[i].parent_pid);
        user_print("    ");
        user_print_dec((int)processes[i].uid);
        user_print("   ");
        user_print_dec((int)processes[i].state);
        user_print("    ");
        user_print_dec((int)processes[i].cpu_ticks);
        user_print("\n");
    }
}

/**
 * @brief The main entry point of the coreutils application.
 * @param argc The argument count
 * @param argv The argument vector
 * @return Exit status code
 */
int main(const int argc, char** argv)
{
    const char* name = app_name(argc, argv);

    if (user_streq(name, "pwd"))
    {
        char path[128];
        if (getcwd(path, sizeof(path)) < 0) return 1;
        user_println(path);
        return 0;
    }
    if (user_streq(name, "mkdir") || user_streq(name, "touch") ||
        user_streq(name, "rm") || user_streq(name, "rmdir"))
    {
        if (argc < 2)
        {
            user_print("usage: ");
            user_print(name);
            user_println(" PATH");
            return 1;
        }
        int operation = FS_OP_REMOVE;
        if (user_streq(name, "mkdir")) operation = FS_OP_MKDIR;
        if (user_streq(name, "touch")) operation = FS_OP_TOUCH;
        if (fs_mutate(operation, argv[1]) < 0)
        {
            user_print(name);
            user_println(": operation failed");
            return 1;
        }
        return 0;
    }
    if (user_streq(name, "ps"))
    {
        print_processes();
        return 0;
    }
    if (user_streq(name, "kill"))
    {
        if (argc < 2) return 1;
        return kill(parse_int(argv[1]), 143) < 0;
    }
    if (user_streq(name, "uptime"))
    {
        user_print("uptime ticks: ");
        user_print_dec(uptime_ticks());
        user_print("\n");
        return 0;
    }
    if (user_streq(name, "mem"))
    {
        struct system_info info;
        if (sysinfo(&info) < 0) return 1;
        user_print("memory KB total=");
        user_print_dec((int)info.total_memory_kb);
        user_print(" used=");
        user_print_dec((int)info.used_memory_kb);
        user_print(" free=");
        user_print_dec((int)info.free_memory_kb);
        user_print("\n");
        return 0;
    }
    if (user_streq(name, "date"))
    {
        struct rtc_time now;
        if (gettime(&now) < 0) return 1;
        user_print_dec(now.year);
        user_print("-");
        user_print_dec(now.month);
        user_print("-");
        user_print_dec(now.day);
        user_print(" ");
        user_print_dec(now.hour);
        user_print(":");
        user_print_dec(now.minute);
        user_print(":");
        user_print_dec(now.second);
        user_print("\n");
        return 0;
    }
    if (user_streq(name, "whoami"))
    {
        user_print("uid ");
        user_print_dec(getuid());
        user_print("\n");
        return 0;
    }
    if (user_streq(name, "sync"))
    {
        return fs_mutate(FS_OP_SYNC, "") < 0;
    }
    if (user_streq(name, "reboot"))
    {
        return power_control(POWER_REBOOT);
    }
    if (user_streq(name, "shutdown"))
    {
        return power_control(POWER_SHUTDOWN);
    }
    if (user_streq(name, "clear"))
    {
        return user_clear("\f") != 1;
    }
    if (user_streq(name, "version") || user_streq(name, "ver"))
    {
        user_println("mexOS userland 0.2");
        return 0;
    }

    user_print(name);
    user_println(": not implemented");
    return 1;
}
