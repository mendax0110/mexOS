#include "init.h"
#include "runtime.h"

int main(void)
{
    user_print("[init] mexOS init process started (user-mode)\n");
    user_print("[init] PID: ");
    user_print_dec(getpid());
    user_print("\n");

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
        return 42;
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
    }
    else
    {
        user_print("[init] Fork failed!\n");
    }

    user_print("[init] Init complete\n");
    return 0;
}
