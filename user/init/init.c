#include "init.h"
#include "runtime.h"

static void run_fork_smoke_test(void)
{
    user_print("[init] Testing fork()...\n");
    const int child = fork();

    if (child == 0)
    {
        user_print("[child] Running in user-mode, PID: ");
        user_print_dec(getpid());
        user_print("\n");

        for (int i = 0; i < 3; i++)
        {
            user_print("[child] tick ");
            user_print_dec(i);
            user_print("\n");
            yield();
        }

        user_print("[child] Exiting with code 42\n");
        exit(42);
    }

    if (child > 0)
    {
        user_print("[init] Created child PID: ");
        user_print_dec(child);
        user_print("\n");

        int status = 0;
        const int result = wait(child, &status);

        user_print("[init] Child exited, PID: ");
        user_print_dec(result);
        user_print(", status: ");
        user_print_dec(status);
        user_print("\n");
        return;
    }

    user_print("[init] Fork failed!\n");
}

int main(const int argc, char** argv)
{
    user_print("[init] mexOS init process started (user-mode)\n");
    user_print("[init] PID: ");
    user_print_dec(getpid());
    user_print("\n");

    if (argc > 1 && user_streq(argv[1], "--forktest"))
    {
        run_fork_smoke_test();
    }

    const bool desktop_mode = argc > 1 && user_streq(argv[1], "--desktop");

    if (!desktop_mode)
    {
        user_print("[init] Starting interactive console\n");
        const char* shell_argv[] = { "sh", NULL };
        if (execv("/bin/sh", shell_argv) < 0)
        {
            user_print("[init] Failed to exec /bin/sh\n");
            return 1;
        }
    }

    user_print("[init] Starting display server and desktop session\n");

    const int display_pid = fork();
    if (display_pid == 0)
    {
        const char* display_argv[] = { "displayd", NULL };
        execv("/bin/displayd", display_argv);
        exit(127);
    }
    if (display_pid < 0)
    {
        user_println("[init] Failed to fork display server");
        return 1;
    }

    for (int i = 0; i < 64; i++) yield();

    int desktop_pid = fork();
    if (desktop_pid == 0)
    {
        const char* desktop_argv[] = { "desktop", NULL };
        execv("/bin/desktop", desktop_argv);
        exit(127);
    }
    if (desktop_pid < 0)
    {
        kill(display_pid, 143);
        return 1;
    }

    while (1)
    {
        int status = 0;
        const int exited = wait(-1, &status);
        if (exited == display_pid)
        {
            user_println("[init] Display server exited; stopping desktop");
            kill(desktop_pid, 143);
            wait(desktop_pid, &status);
            const char* shell_argv[] = { "sh", NULL };
            execv("/bin/sh", shell_argv);
            return 1;
        }
        if (exited == desktop_pid)
        {
            desktop_pid = fork();
            if (desktop_pid == 0)
            {
                const char* desktop_argv[] = { "desktop", NULL };
                execv("/bin/desktop", desktop_argv);
                exit(127);
            }
        }
    }
}
