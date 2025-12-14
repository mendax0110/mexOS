#ifndef KERNEL_DISKFS_H
#define KERNEL_DISKFS_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISKFS_MAGIC 0x6D786673U   // "mxfs1" in hex
#define DISKFS_VERSION 1
#define DISKFS_SECTOR_SIZE 512
#define DISKFS_BLOCK_SIZE 512

// Disk layout constants
#define DISKFS_SUPERBLOCK_SECTOR 0
#define DISKFS_INODE_BITMAP_START 1
#define DISKFS_INODE_BITMAP_SECTORS 16
#define DISKFS_BLOCK_BITMAP_START 17
#define DISKFS_BLOCK_BITMAP_SECTORS 16
#define DISKFS_INODE_TABLE_START 33
#define DISKFS_INODE_TABLE_SECTORS 512
#define DISKFS_DATA_START 545

// Limits
#define DISKFS_MAX_INODES 512     // Limited by inode table size
#define DISKFS_MAX_BLOCKS 65536   // Limited by bitmap
#define DISKFS_MAX_FILENAME 28      // Fits in dir entry with inode
#define DISKFS_DIRECT_BLOCKS 12      // Direct block pointers in inode
#define DISKFS_MAX_FILE_SIZE  (DISKFS_DIRECT_BLOCKS * DISKFS_BLOCK_SIZE)

// File types
#define DISKFS_TYPE_FREE 0
#define DISKFS_TYPE_FILE 1
#define DISKFS_TYPE_DIR 2

/**
 * @brief Superblock structure (fits in 512-byte sector) \struct diskfs_superblock
 */
struct diskfs_superblock
{
    uint32_t magic;
    uint32_t version;
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t free_inodes;
    uint32_t free_blocks;
    uint32_t root_inode;
    uint32_t block_size;
    uint8_t pad[480];
} __attribute__((packed));

/**
 * @brief Inode structure (128 bytes) \struct diskfs_inode
 */
struct diskfs_inode
{
    uint32_t type;
    uint32_t size;
    uint32_t blocks[DISKFS_DIRECT_BLOCKS];
    uint32_t ctime;
    uint32_t mtime;
    uint32_t parent_inode;
    uint8_t pad[68];
} __attribute__((packed));

/**
 * @brief Directory entry structure (32 bytes) \struct diskfs_dirent
 */
struct diskfs_dirent
{
    uint32_t inode;
    char     name[DISKFS_MAX_FILENAME];
} __attribute__((packed));

/**
 * @brief Initialize diskfs subsystem
 * @param drive ATA drive number (0 = primary master)
 * @return 0 on success, negative on error
 */
int diskfs_init(uint8_t drive);

/**
 * @brief Format a disk with mexFS filesystem
 * @param drive ATA drive number
 * @return 0 on success, negative on error
 */
int diskfs_format(uint8_t drive);

/**
 * @brief Mount a mexFS filesystem
 * @param drive ATA drive number
 * @return 0 on success, negative on error
 */
int diskfs_mount(uint8_t drive);

/**
 * @brief Unmount the current filesystem (flush all caches)
 * @return 0 on success, negative on error
 */
int diskfs_unmount(void);

/**
 * @brief Sync filesystem (write all cached data to disk)
 * @return 0 on success, negative on error
 */
int diskfs_sync(void);

/**
 * @brief Create a file or directory
 * @param parent_ino Parent directory inode number
 * @param name Filename
 * @param type File type (DISKFS_TYPE_FILE or DISKFS_TYPE_DIR)
 * @return Inode number on success, negative on error
 */
int diskfs_create(uint32_t parent_ino, const char* name, uint32_t type);

/**
 * @brief Delete a file or directory
 * @param parent_ino Parent directory inode number
 * @param name Filename
 * @return 0 on success, negative on error
 */
int diskfs_delete(uint32_t parent_ino, const char* name);

/**
 * @brief Read data from a file
 * @param ino Inode number
 * @param buffer Buffer to read into
 * @param offset Offset in file
 * @param size Number of bytes to read
 * @return Number of bytes read, or negative on error
 */
int diskfs_read(uint32_t ino, void* buffer, uint32_t offset, uint32_t size);

/**
 * @brief Write data to a file
 * @param ino Inode number
 * @param buffer Buffer to write from
 * @param offset Offset in file
 * @param size Number of bytes to write
 * @return Number of bytes written, or negative on error
 */
int diskfs_write(uint32_t ino, const void* buffer, uint32_t offset, uint32_t size);

/**
 * @brief Lookup a file in a directory
 * @param dir_ino Directory inode number
 * @param name Filename to look up
 * @return Inode number on success, negative on error
 */
int diskfs_lookup(uint32_t dir_ino, const char* name);

/**
 * @brief Read directory entries
 * @param dir_ino Directory inode number
 * @param entries Buffer to store directory entries
 * @param max_entries Maximum number of entries to read
 * @return Number of entries read, or negative on error
 */
int diskfs_readdir(uint32_t dir_ino, struct diskfs_dirent* entries, uint32_t max_entries);

/**
 * @brief Get inode information
 * @param ino Inode number
 * @param inode Buffer to store inode data
 * @return 0 on success, negative on error
 */
int diskfs_stat(uint32_t ino, struct diskfs_inode* inode);

/**
 * @brief Check if filesystem is mounted
 * @return 1 if mounted, 0 if not
 */
int diskfs_is_mounted(void);

#ifdef __cplusplus
}
#endif

#endif // KERNEL_DISKFS_H
