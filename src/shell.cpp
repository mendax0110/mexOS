#include "userlib.h"
#include "kernelUtils.h"

#define MAX_INPUT_LENGTH 128
#define MAX_ARGS 8

void print_prompt()
{
    print("mexOS> ");
}

void print_help()
{
    print("Available commands:\n");
    print("  help    - Show this help\n");
    print("  tasks   - List running tasks\n");
    print("  version - Show OS version\n");
    print("  clear   - Clear screen\n");
}

void list_tasks()
{
    int task_count = syscall(2);
    print("Running tasks: ");
    print(int_to_str(task_count));
    print("\n");

    for (int i = 0; i < task_count; i++)
    {
        print("  Task ");
        print(int_to_str(i));
        print("\n");
    }
}

void show_version()
{
    int version = syscall(4);
    print("mexOS version: 0.");
    print(int_to_str(version));
    print("\n");
}

void clear_screen()
{
    for (int i = 0; i < 24; i++)
    {
        print("\n");
    }
}

void execute_command(const char* cmd)
{
    if (strcmp(cmd, "help") == 0)
    {
        print_help();
    }
    else if (strcmp(cmd, "tasks") == 0)
    {
        list_tasks();
    }
    else if (strcmp(cmd, "version") == 0)
    {
        show_version();
    }
    else if (strcmp(cmd, "clear") == 0)
    {
        clear_screen();
    }
    else if (strlen(cmd) > 0)
    {
        print("Unknown command: ");
        print(cmd);
        print("\nType 'help' for available commands\n");
    }
}

void shell()
{
    char input[MAX_INPUT_LENGTH];
    uint32_t pos = 0;

    clear_screen();
    print("mexOS Shell - Built-in Command Line\n");
    print("----------------------------------\n");

    while (1)
    {
        print_prompt();
        pos = 0;

        for (volatile int i = 0; i < 1000000; i++);

        static const char* demo_cmds[] = {"help", "tasks", "version", "clear"};
        static uint32_t cmd_index = 0;

        strcpy(input, demo_cmds[cmd_index]);
        cmd_index = (cmd_index + 1) % 4;

        print(input);
        print("\n");

        execute_command(input);
    }
}

void user_program()
{
    shell();
}