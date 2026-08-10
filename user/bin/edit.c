#include "runtime.h"

#define EDIT_MAX_FILE_SIZE 4096
#define EDIT_LINE_SIZE 256
#define EDIT_FILE_LINE_SIZE 128

static char g_buffer[EDIT_MAX_FILE_SIZE];
static uint32_t g_buffer_size = 0;
static char g_filename[EDIT_FILE_LINE_SIZE];
static int g_modified = 0;

static int edit_load(const char* path)
{
    struct fs_stat st;
    const int existed = stat(path, &st) == 0;

    if (!existed)
    {
        if (fs_mutate(FS_OP_TOUCH, path) < 0)
        {
            user_println("edit: cannot create file");
            return -1;
        }
    }

    const int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        user_println("edit: cannot open file");
        return -1;
    }

    g_buffer_size = 0;
    while (g_buffer_size < EDIT_MAX_FILE_SIZE - 1)
    {
        const int n = read(fd, g_buffer + g_buffer_size, EDIT_MAX_FILE_SIZE - 1 - g_buffer_size);
        if (n <= 0)
        {
            break;
        }
        g_buffer_size += (uint32_t)n;
    }

    g_buffer[g_buffer_size] = '\0';
    close(fd);
    return 0;
}

static int edit_save(void)
{
    fs_mutate(FS_OP_REMOVE, g_filename);
    if (fs_mutate(FS_OP_TOUCH, g_filename) < 0)
    {
        user_println("edit: cannot create file");
        return -1;
    }

    const int fd = open(g_filename, O_WRONLY);
    if (fd < 0)
    {
        user_println("edit: cannot open file for writing");
        return -1;
    }

    const int n = write(fd, g_buffer, (int)g_buffer_size);
    close(fd);
    if (n < 0)
    {
        user_println("edit: write failed");
        return -1;
    }

    g_modified = 0;
    return 0;
}

static int edit_readline(char* out, const int max)
{
    int pos = 0;
    while (true)
    {
        char c;
        const int n = read(STDIN_FILENO, &c, 1);
        if (n <= 0) continue;

        if (c == '\n' || c == '\r')
        {
            write(STDOUT_FILENO, "\n", 1);
            out[pos] = '\0';
            return pos;
        }

        if (c == '\b' || c == 0x7F)
        {
            if (pos > 0)
            {
                pos--;
                write(STDOUT_FILENO, "\b \b", 3);
            }
            continue;
        }

        if (c >= 0x20 && c < 0x7F && pos < max - 1)
        {
            out[pos++] = c;
            write(STDOUT_FILENO, &c, 1);
        }
    }
}

static void edit_add_line(const char* line)
{
    const uint32_t buf_len = g_buffer_size;
    const uint32_t line_len = user_strlen(line);

    if (buf_len + line_len + 2 >= EDIT_MAX_FILE_SIZE)
    {
        user_println("edit: buffer full, cannot add line");
        return;
    }

    user_memcpy(g_buffer + buf_len, line, line_len);
    g_buffer[buf_len + line_len] = '\n';
    g_buffer_size = buf_len + line_len + 1;
    g_buffer[g_buffer_size] = '\0';
    g_modified = 1;
}

static void edit_print_buffer(void)
{
    user_println("---");
    if (g_buffer_size > 0)
    {
        write(STDOUT_FILENO, g_buffer, (int)g_buffer_size);
        if (g_buffer[g_buffer_size - 1] != '\n')
        {
            user_putc('\n');
        }
    }
    user_println("---");
}

int main(const int argc, char** argv)
{
    if (argc < 2)
    {
        user_println("Usage: edit <filename>");
        return 1;
    }

    user_memcpy(g_filename, argv[1], user_strlen(argv[1]) + 1);

    if (edit_load(g_filename) < 0)
    {
        return 1;
    }

    user_print("=== edit: ");
    user_print(g_filename);
    user_println(" === (:w save, :q quit, :wq both, :p print, :h help)");

    char line[EDIT_LINE_SIZE];
    while (true)
    {
        user_print("> ");
        edit_readline(line, EDIT_LINE_SIZE);

        if (user_streq(line, ":q"))
        {
            if (g_modified)
            {
                user_println("Warning: unsaved changes (use :q! to force quit)");
                continue;
            }

            break;
        }

        if (user_streq(line, ":q!")) break;
        if (user_streq(line, ":w"))
        {
            if (edit_save() < 0)
            {
                user_println("edit: save failed");
            }
            else
            {
                user_println("File saved");
            }

            continue;
        }

        if (user_streq(line, ":wq"))
        {
            if (edit_save() < 0)
            {
                user_println("edit: save failed");
                continue;
            }

            break;
        }

        if (user_streq(line, ":p"))
        {
            edit_print_buffer();
            continue;
        }

        if (user_streq(line, ":h"))
        {
            user_println(":q quit | :q! force quit | :w save | :wq save and quit | :p print buffer | :h help");
            continue;
        }

        edit_add_line(line);
    }

    return 0;
}