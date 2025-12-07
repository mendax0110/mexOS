#include "../lib/syscall.h"
#include "init.h"

static void print(const char* str)
{
    int len = 0;
    while (str[len])
    {
        len++;
    }
    write(str, len);
}

static void print_dec(int num)
{
    if (num == 0)
    {
        write("0", 1);
        return;
    }

    char buf[12];
    int i = 0;
    int neg = 0;

    if (num < 0)
    {
        neg = 1;
        num = -num;
    }

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (neg)
    {
        buf[i++] = '-';
    }

    char out[12];
    int j = 0;
    while (i > 0)
    {
        out[j++] = buf[--i];
    }
    write(out, j);
}

int main(void)
{
    print("[init] mexOS init process started (user-mode)\n");
    print("[init] PID: ");
    print_dec(getpid());
    print("\n");

    print("[init] Testing fork()...\n");
    int child = fork();

    if (child == 0)
    {
        print("[child] Running in user-mode, PID: ");
        print_dec(getpid());
        print("\n");

        for (int i = 0; i < 3; i++)
        {
            print("[child] tick ");
            print_dec(i);
            print("\n");
            yield();
        }

        print("[child] Exiting with code 42\n");
        return 42;
    }
    else if (child > 0)
    {
        print("[init] Created child PID: ");
        print_dec(child);
        print("\n");

        int status = 0;
        int result = wait(child, &status);

        print("[init] Child exited, PID: ");
        print_dec(result);
        print(", status: ");
        print_dec(status);
        print("\n");
    }
    else
    {
        print("[init] Fork failed!\n");
    }

    print("[init] Init complete\n");
    return 0;
}
