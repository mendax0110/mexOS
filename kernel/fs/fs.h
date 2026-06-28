#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include "include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple in-memory filesystem definitions
 */
#define FS_MAX_NAME         32
#define FS_MAX_PATH         128
#define FS_MAX_FILES        64
#define FS_MAX_FILE_SIZE    4096
#define FS_MAX_DIR_ENTRIES  16
#define FS_MAX_PATH_DEPTH   8

#define FS_TYPE_FILE        0
#define FS_TYPE_DIR         1

#define FS_ERR_OK           0
#define FS_ERR_NOT_FOUND    (-1)
#define FS_ERR_EXISTS       (-2)
#define FS_ERR_FULL         (-3)
#define FS_ERR_INVALID      (-4)
#define FS_ERR_NOT_EMPTY    (-5)
#define FS_ERR_IS_DIR       (-6)
#define FS_ERR_NOT_DIR      (-7)

/// @brief Filesystem node structure \struct fs_node
struct fs_node
{
    char name[FS_MAX_NAME];
    uint8_t type;
    uint8_t used;
    uint32_t size;
    uint32_t parent_idx;
    uint8_t data[FS_MAX_FILE_SIZE];
};

/**
 * @brief Initialize the filesystem
 */
void fs_init(void);

/**
 * @brief Create a file at the specified path
 * @param path The path of the file to create
 * @return FS_ERR_OK on success, or a negative error code
 */
int fs_create_file(const char* path);

/**
 * @brief Create a directory at the specified path
 * @param path The path of the directory to create
 * @return FS_ERR_OK on success, or a negative error code
 */
int fs_create_dir(const char* path);

/**
 * @brief Remove a file or directory at the specified path
 * @param path The path of the file or directory to remove
 * @return FS_ERR_OK on success, or a negative error code
 */
int fs_remove(const char* path);

/**
 * @brief Read data from a file at the specified path
 * @param path The path of the file to read
 * @param buffer The buffer to read data into
 * @param size The maximum number of bytes to read
 * @return The number of bytes read, or a negative error code
 */
int fs_read(const char* path, char* buffer, uint32_t size);

/**
 * @brief Write data to a file at the specified path
 * @param path The path of the file to write to
 * @param data The data to write
 * @param size The number of bytes to write
 * @return FS_ERR_OK on success, or a negative error code
 */
int fs_write(const char* path, const char* data, uint32_t size);

/**
 * @brief Append data to a file at the specified path
 * @param path The path of the file to append to
 * @param data The data to append
 * @param size The number of bytes to append
 * @return FS_ERR_OK on success, or a negative error code
 */
int fs_append(const char* path, const char* data, uint32_t size);

/**
 * @brief List the contents of a directory at the specified path
 * @param path The path of the directory to list
 * @param buffer The buffer to write the directory listing into
 * @param size The maximum size of the buffer
 * @return The number of bytes written to the buffer, or a negative error code
 */
int fs_list_dir(const char* path, char* buffer, uint32_t size);

/**
 * @brief Change the current working directory
 * @param path The path of the directory to change to
 * @return FS_ERR_OK on success, or a negative error code
 */
int fs_change_dir(const char* path);

/**
 * @brief Get the current working directory
 * @return The current working directory path
 */
const char* fs_get_cwd(void);

/**
 * @brief Check if a file or directory exists at the specified path
 * @param path The path to check
 * @return 1 if exists, 0 if not
 */
int fs_exists(const char* path);

/**
 * @brief Check if the path is a directory
 * @param path The path to check
 * @return 1 if directory, 0 if not
 */
int fs_is_dir(const char* path);

/**
 * @brief Get the size of a file at the specified path
 * @param path The path of the file
 * @return The size of the file in bytes
 */
uint32_t fs_get_size(const char* path);

/**
 * @brief Clear the filesystem cache (if any)
 */
void fs_clear_cache(void);

/**
 * @brief Enable disk-based filesystem on the specified drive
 * @param drive The drive number to enable
 * @return FS_ERR_OK on success, or a negative error code
 */
int fs_enable_disk(uint8_t drive);

/**
 * @brief Disable the disk-based filesystem
 * @return FS_ERR_OK on success, or a negative error code
 */
int fs_sync(void);

/**
 * @brief Check if disk-based filesystem is enabled
 * @return 1 if enabled, 0 if not
 */
int fs_is_disk_enabled(void);

#ifdef __cplusplus
}
#endif

#endif
