#include "runtime.h"
#include "../shared/fs_abi.h"

#define TREE_MAX_ENTRIES 64
#define TREE_MAX_PATH 256
#define TREE_MAX_PREFIX 256

/**
 * @brief Build a path string by combining a parent directory and a child name
 * @param out The output buffer to store the combined path
 * @param out_size The size of the output buffer
 * @param parent The parent directory
 * @param name The child name
 * @return The length of the combined path
 */
static int build_path(char* out, const int out_size, const char* parent, const char* name)
{
    const size_t parent_len = copy_string(out, (size_t)out_size, parent);
    if (parent_len > 0 && out[parent_len - 1] != '/' && parent_len + 1 < (size_t)out_size)
    {
        out[parent_len] = '/';
        out[parent_len + 1] = '\0';
    }

    return (int)append_string(out, (size_t)out_size, name);
}

/**
 * @brief Extend the prefix string for tree display based on whether the parent was the last entry
 * @param out The output buffer to store the extended prefix
 * @param out_size The size of the output buffer
 * @param parent_prefix The current prefix
 * @param parent_was_last Whether the parent was the last entry
 * @return The length of the extended prefix
 */
static int extend_prefix(char* out, const int out_size, const char* parent_prefix, const int parent_was_last)
{
    const char* pad = parent_was_last ? "    " : "|   ";
    copy_string(out, (size_t)out_size, parent_prefix);
    return (int)append_string(out, (size_t)out_size, pad);
}

/**
 * @brief Recursively print the directory tree starting from a given entry
 * @param entry The directory entry to print
 * @param path The current path of the entry
 * @param prefix The prefix string for formatting
 * @param is_last The flag indicating if this entry is the last in its directory
 * @param is_root The flag indicating if this entry is the root of the tree
 */
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

/**
 * @brief The main entry point of the tree executable
 * @param argc The argument count
 * @param argv The argument vector
 * @return Exit status code
 */
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