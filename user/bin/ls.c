#include "runtime.h"

#define LS_MAX_ENTRIES 64

static void print_entry(const struct fs_dirent* entry)
{
    if (entry->type == FS_ABI_TYPE_DIR)
    {
        user_putc('[');
        user_print(entry->name);
        user_putc(']');
    }
    else
    {
        user_print(entry->name);
    }
    user_putc('\n');
}

static int list_path(const char* path)
{
    struct fs_dirent entries[LS_MAX_ENTRIES];
    const int count = readdir(path, entries, LS_MAX_ENTRIES);
    if (count >= 0)
    {
        if (count == 0)
        {
            user_println("(empty)");
            return 0;
        }

        for (int i = 0; i < count; i++)
        {
            print_entry(&entries[i]);
        }
        return 0;
    }

    struct fs_stat info;
    if (stat(path, &info) == 0 && info.type == FS_ABI_TYPE_FILE)
    {
        user_println(path);
        return 0;
    }

    user_print("ls: cannot access ");
    user_println(path);
    return 1;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return list_path(".");
    }

    int rc = 0;
    for (int i = 1; i < argc; i++)
    {
        if (argc > 2)
        {
            if (i > 1)
            {
                user_putc('\n');
            }

            user_print(argv[i]);
            user_println(":");
        }

        if (list_path(argv[i]) != 0)
        {
            rc = 1;
        }
    }

    return rc;
}
