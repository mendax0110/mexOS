#ifndef SHARED_FS_ABI_H
#define SHARED_FS_ABI_H

#define FS_ABI_NAME_MAX 32

#define FS_ABI_TYPE_FILE 0
#define FS_ABI_TYPE_DIR  1

/**
 * @brief Struct representing filesystem stats as type and size \struct fs_stat
 */
struct fs_stat
{
    uint32_t type;
    uint32_t size;
};

/**
 * @brief Struct representing a filesystem directory entry \struct fs_dirent
 */
struct fs_dirent
{
    char name[FS_ABI_NAME_MAX];
    uint32_t type;
    uint32_t size;
};

#endif
