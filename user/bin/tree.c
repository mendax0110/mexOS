#include "runtime.h"
#include "../shared/fs_abi.h"

#define TREE_MAX_ENTRIES 64
#define TREE_MAX_PATH 256
#define TREE_MAX_PREFIX 256

static int build_path(char* out, const int out_size, const char* parent, const char* name)
{
    int len = 0;

    while (parent[len] != '\0' && len < out_size - 1)
    {
        out[len] = parent[len];
        len++;
    }

    if (len > 0 && out[len - 1] != '/' && len < out_size - 1)
    {
        out[len++] = '/';
    }

    int i = 0;
    while (name[i] != '\0' && len < out_size - 1)
    {
        out[len++] = name[i++];
    }

    out[len] = '\0';
    return len;
}

static int extend_prefix(char* out, const int out_size, const char* parent_prefix, const int parent_was_last)
{
    int len = 0;
    while (parent_prefix[len] != '\0' && len < out_size - 1)
    {
        out[len] = parent_prefix[len];
        len++;
    }

    const char* pad = parent_was_last ? "    " : "|   ";
    for (int i = 0; pad[i] != '\0' && len < out_size - 1; i++)
    {
        out[len++] = pad[i];
    }

    out[len] = '\0';
    return len;
}

static void print_tree(const struct fs_dirent* entry, const char* path, const char* prefix, const int is_last, const int is_root)
{
    if (!entry) return;

    if (is_root)
    {
        user_print(entry->name);
        user_putc('/');
        user_putc('\n');
    }
    else
    {
        user_print(prefix);
        user_print(is_last ? "`-- " : "|-- ");
        user_print(entry->name);
        if (entry->type == FS_ABI_TYPE_DIR)
        {
            user_putc('/');
        }
        user_putc('\n');
    }

    if (entry->type == FS_ABI_TYPE_DIR)
    {
        struct fs_dirent sub_entries[TREE_MAX_ENTRIES];
        const int count = readdir(path, sub_entries, TREE_MAX_ENTRIES);

        if (count > 0)
        {
            char child_prefix[TREE_MAX_PREFIX];

            if (is_root)
            {
                child_prefix[0] = '\0';
            }
            else
            {
                extend_prefix(child_prefix, TREE_MAX_PREFIX, prefix, is_last);
            }

            for (int i = 0; i < count; i++)
            {
                char sub_path[TREE_MAX_PATH];
                build_path(sub_path, TREE_MAX_PATH, path, sub_entries[i].name);
                print_tree(&sub_entries[i], sub_path, child_prefix, i == count - 1, 0);
            }
        }
    }
}

int main(const int argc, char** argv)
{
    const char* path = (argc > 1) ? argv[1] : ".";

    struct fs_stat info;
    if (stat(path, &info) != 0)
    {
        user_print("tree: cannot access ");
        user_println(path);
        return 1;
    }

    struct fs_dirent root;
    root.type = info.type;

    int i = 0;
    while (path[i] != '\0' && i < (int)sizeof(root.name) - 1)
    {
        root.name[i] = path[i];
        i++;
    }
    root.name[i] = '\0';

    print_tree(&root, path, "", 1, 1);

    return 0;
}