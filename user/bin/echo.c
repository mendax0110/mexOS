#include "runtime.h"

int main(const int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        user_print(argv[i]);
        if (i + 1 < argc)
        {
            user_putc(' ');
        }
    }

    user_putc('\n');
    return 0;
}
