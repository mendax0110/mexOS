#ifndef SERVERS_VFS_PROTOCOL_H
#define SERVERS_VFS_PROTOCOL_H

#include "../../include/protocols/vfs_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of open files per process
 */
#define VFS_MAX_FDS 32

/**
 * @brief Maximum number of filesystem nodes
 */
#define VFS_MAX_NODES 256

/**
 * @brief Maximum file size
 */
#define VFS_MAX_FILE_SIZE (1024 * 1024)  /* 1MB */

/**
 * @brief VFS error codes \enum vfs_error
 */
enum vfs_error
{
    VFS_ERR_OK =  0,
    VFS_ERR_INVALID = -1,
    VFS_ERR_NOTFOUND = -2,
    VFS_ERR_EXISTS = -3,
    VFS_ERR_NOSPACE = -4,
    VFS_ERR_NOTDIR = -5,
    VFS_ERR_ISDIR = -6,
    VFS_ERR_NOTEMPTY = -7,
    VFS_ERR_NOPERM = -8,
    VFS_ERR_BUSY = -9,
    VFS_ERR_IO = -10,
    VFS_ERR_NOMEM = -11
};

/**
 * @brief Internal filesystem node structure \struct vfs_node
 */
struct vfs_node
{
    char name[VFS_MAX_NAME];
    uint8_t type;
    uint8_t used;
    uint8_t reserved[2];
    uint32_t size;
    uint32_t parent_idx;
    uint32_t data_sector;
    uint32_t created;
    uint32_t modified;
    uint8_t  *data;
};

/**
 * @brief File descriptor structure \struct vfs_fd
 */
struct vfs_fd
{
    pid_t owner;
    int32_t node_idx;
    uint32_t position;
    uint16_t flags;
    uint8_t used;
    uint8_t reserved;
};

/**
 * @brief Process working directory structure \struct vfs_cwd
 */
struct vfs_cwd
{
    pid_t pid;
    uint32_t dir_idx;
    char path[VFS_MAX_PATH];
};

#ifdef __cplusplus
}
#endif

#endif // SERVERS_VFS_PROTOCOL_H