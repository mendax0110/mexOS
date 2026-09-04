#include "runtime.h"

/**
 * @brief The main function of the echo application.
 * It prints the command line arguments to the standard output, separated by spaces.
 * @param argc The number of command line arguments
 * @param argv The array of command line argument strings
 * @return Exit status code
 */
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
