#include "runtime.h"

#define CAT_BUFFER_SIZE 256

/**
 * @brief Read the contents of a file and print it to standard output.
 * @param path The path to the file to read.
 * @return 0 on success, 1 on failure.
 */
static int cat_file(const char* path)
{
    const int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        user_print("cat: cannot open ");
        user_println(path);
        return 1;
    }

    char buffer[CAT_BUFFER_SIZE];
    while (1)
    {
        const int bytes_read = read(fd, buffer, CAT_BUFFER_SIZE);
        if (bytes_read < 0)
        {
            user_print("cat: read failed for ");
            user_println(path);
            close(fd);
            return 1;
        }

        if (bytes_read == 0)
        {
            break;
        }

        write(STDOUT_FILENO, buffer, bytes_read);
    }

    close(fd);
    return 0;
}

/**
 * @brief The main entry point of the cat application.
 * @param argc The argument count.
 * @param argv The argument vector.
 * @return Exit status code.
 */
int main(const int argc, char** argv)
{
    if (argc < 2)
    {
        user_println("cat: missing file operand");
        return 1;
    }

    int rc = 0;
    for (int i = 1; i < argc; i++)
    {
        if (cat_file(argv[i]) != 0)
        {
            rc = 1;
        }
    }

    return rc;
}
