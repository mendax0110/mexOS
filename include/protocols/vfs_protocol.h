/**
 * @file vfs_protocol.h
 * @brief VFS (Virtual File System) server IPC protocol definitions
 *
 * This header defines the message types and structures for communication
 * with the VFS server in the mexOS microkernel architecture.
 */

#ifndef INCLUDE_VFS_PROTOCOL_H
#define INCLUDE_VFS_PROTOCOL_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VFS server port name for nameserver registration
 */
#define VFS_SERVER_PORT_NAME "vfs"

/**
 * @brief Maximum path length
 */
#define VFS_MAX_PATH 256

/**
 * @brief Maximum filename length
 */
#define VFS_MAX_NAME 64

/**
 * @brief Maximum data transfer per message
 */
#define VFS_MAX_DATA 192

/**
 * @brief VFS message types \enum vfs_msg_type
 */
enum vfs_msg_type
{
    VFS_MSG_OPEN = 0x0400,
    VFS_MSG_CLOSE= 0x0401,
    VFS_MSG_READ = 0x0402,
    VFS_MSG_WRITE = 0x0403,
    VFS_MSG_SEEK = 0x0404,
    VFS_MSG_STAT = 0x0405,
    VFS_MSG_MKDIR = 0x0406,
    VFS_MSG_RMDIR = 0x0407,
    VFS_MSG_UNLINK = 0x0408,
    VFS_MSG_RENAME = 0x0409,
    VFS_MSG_READDIR = 0x040A,
    VFS_MSG_CHDIR = 0x040B,
    VFS_MSG_GETCWD = 0x040C,
    VFS_MSG_MOUNT = 0x0410,
    VFS_MSG_UMOUNT = 0x0411,
    VFS_MSG_RESPONSE = 0x04FF
};

/**
 * @brief File open flags \enum vfs_open_flags
 */
enum vfs_open_flags
{
    VFS_O_RDONLY = 0x0001,
    VFS_O_WRONLY = 0x0002,
    VFS_O_RDWR = 0x0003,
    VFS_O_CREATE = 0x0100,
    VFS_O_TRUNC = 0x0200,
    VFS_O_APPEND = 0x0400
};

/**
 * @brief File seek modes \enum vfs_seek_mode
 */
enum vfs_seek_mode
{
    VFS_SEEK_SET = 0,
    VFS_SEEK_CUR = 1,
    VFS_SEEK_END = 2
};

/**
 * @brief File types \enum vfs_file_type
 */
enum vfs_file_type
{
    VFS_TYPE_FILE = 0,
    VFS_TYPE_DIR = 1,
    VFS_TYPE_LINK = 2,
    VFS_TYPE_DEV = 3
};

/**
 * @brief VFS open request structure \struct vfs_open_request
 */
struct vfs_open_request
{
    uint16_t flags;
    uint16_t mode;
    char path[VFS_MAX_PATH];
};

/**
 * @brief VFS open response structure \struct vfs_open_response
 */
struct vfs_open_response
{
    int32_t status;
    int32_t fd;
};

/**
 * @brief VFS close request structure \struct vfs_close_request
 */
struct vfs_close_request
{
    int32_t fd;
};

/**
 * @brief VFS read request structure \struct vfs_read_request
 */
struct vfs_read_request
{
    int32_t fd;
    uint32_t size;
    uint32_t offset;
};

/**
 * @brief VFS read response structure \struct vfs_read_response
 */
struct vfs_read_response
{
    int32_t status;
    uint8_t data[VFS_MAX_DATA];
};

/**
 * @brief VFS write request structure \struct vfs_write_request
 */
struct vfs_write_request
{
    int32_t fd;
    uint32_t size;
    uint8_t data[VFS_MAX_DATA];
};

/**
 * @brief VFS write response structure \struct vfs_write_response
 */
struct vfs_write_response
{
    int32_t status;
};

/**
 * @brief VFS seek request structure \struct vfs_seek_request
 */
struct vfs_seek_request
{
    int32_t fd;
    int32_t offset;
    int32_t whence;
};

/**
 * @brief VFS seek response structure \struct vfs_seek_response
 */
struct vfs_seek_response
{
    int32_t status;
    int32_t position;
};

/**
 * @brief File status structure \struct vfs_stat
 */
struct vfs_stat
{
    uint8_t type;
    uint8_t reserved[3];
    uint32_t size;
    uint32_t created;
    uint32_t modified;
    uint32_t accessed;
};

/**
 * @brief VFS stat request structure \struct vfs_stat_request
 */
struct vfs_stat_request
{
    char path[VFS_MAX_PATH];
};

/**
 * @brief VFS stat response structure \struct vfs_stat_response
 */
struct vfs_stat_response
{
    int32_t status;
    struct vfs_stat info;
};

/**
 * @brief Directory entry structure \struct vfs_dirent
 */
struct vfs_dirent
{
    uint8_t type;
    char name[VFS_MAX_NAME];
};

/**
 * @brief VFS readdir response structure \struct vfs_readdir_response
 */
struct vfs_readdir_response
{
    int32_t status;
    uint8_t count;
    uint8_t more;
    struct vfs_dirent entries[3];
};

/**
 * @brief VFS path request structure (for mkdir, rmdir, unlink, chdir) \struct vfs_path_request
 */
struct vfs_path_request
{
    char path[VFS_MAX_PATH];
};

/**
 * @brief VFS rename request structure \struct vfs_rename_request
 */
struct vfs_rename_request
{
    char old_path[VFS_MAX_PATH / 2];
    char new_path[VFS_MAX_PATH / 2];
};

/**
 * @brief VFS getcwd response structure \struct vfs_getcwd_response
 */
struct vfs_getcwd_response
{
    int32_t status;
    char path[VFS_MAX_PATH];
};

/**
 * @brief Generic VFS response structure \struct vfs_response
 */
struct vfs_response
{
    int32_t status;
};

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_VFS_PROTOCOL_H