#include "diskfs.h"
#include "drivers/storage/ata.h"
#include "lib/log.h"
#include "lib/string.h"
#include "sched/timer.h"
#include "ui/console.h"

static struct blockdev_ops mounted_dev_storage;
static const struct blockdev_ops* mounted_dev = NULL;

static struct diskfs_superblock superblock;
static uint8_t inode_bitmap[DISKFS_INODE_BITMAP_SECTORS * DISKFS_SECTOR_SIZE];
static uint8_t block_bitmap[DISKFS_BLOCK_BITMAP_SECTORS * DISKFS_SECTOR_SIZE];

static struct diskfs_inode inode_cache[8];
static uint32_t inode_cache_num[8];
static uint8_t inode_cache_valid[8];

/**
 * @brief diskfs_ata_read_sectors - Read sectors from an ATA device
 * @param ctx The context pointer, which is the ATA device number cast to void*
 * @param lba The logical block address to start reading from
 * @param sector_count The number of sectors to read
 * @param buffer The buffer to read the data into
 * @return 0 on success, negative error code on failure
 */
static int diskfs_ata_read_sectors(void* ctx, const uint32_t lba, const uint8_t sector_count, void* buffer)
{
    return ata_read_sectors((uint8_t)(uintptr_t)ctx, lba, sector_count, buffer);
}

/**
 * @brief diskfs_ata_write_sectors - Write sectors to an ATA device
 * @param ctx The context pointer, which is the ATA device number cast to void*
 * @param lba The logical block address to start writing to
 * @param sector_count The number of sectors to write
 * @param buffer The buffer containing the data to write
 * @return 0 on success, negative error code on failure
 */
static int diskfs_ata_write_sectors(void* ctx, const uint32_t lba, const uint8_t sector_count, void* buffer)
{
    return ata_write_sectors((uint8_t)(uintptr_t)ctx, lba, sector_count, buffer);
}

/**
 * @brief bitmap_set - Set a bit in a bitmap
 * @param bitmap The bitmap to modify
 * @param bit The bit to set
 */
static void bitmap_set(uint8_t* bitmap, const uint32_t bit)
{
    bitmap[bit / 8] |= (1 << (bit % 8));
}

/**
 * @brief bitmap_clear - Clear a bit in a bitmap
 * @param bitmap The bitmap to modify
 * @param bit The bit to clear
 */
static void bitmap_clear(uint8_t* bitmap, const uint32_t bit)
{
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

/**
 * @brief bitmap_test - Test if a bit is set in a bitmap
 * @param bitmap The bitmap to test
 * @param bit The bit to test
 * @return 1 if the bit is set, 0 if it is clear
 */
static int bitmap_test(const uint8_t* bitmap, const uint32_t bit)
{
    return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

/**
 * @brief bitmap_find_free - Find the first free bit in a bitmap
 * @param bitmap The bitmap to search
 * @param max_bits The maximum number of bits to search
 * @return The index of the first free bit, or -1 if none are free
 */
static int bitmap_find_free(const uint8_t* bitmap, const uint32_t max_bits)
{
    for (uint32_t i = 0; i < max_bits; i++)
    {
        if (!bitmap_test(bitmap, i))
        {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief read_superblock - Read the superblock from the mounted device
 * @return 0 on success, negative error code on failure
 */
static int read_superblock()
{
    return mounted_dev->read_sectors(mounted_dev->ctx, DISKFS_SUPERBLOCK_SECTOR, 1, &superblock);
}

/**
 * @brief write_superblock - Write the superblock to the mounted device
 * @return 0 on success, negative error code on failure
 */
static int write_superblock()
{
    return mounted_dev->write_sectors(mounted_dev->ctx, DISKFS_SUPERBLOCK_SECTOR, 1, &superblock);
}

/**
 * @brief read_bitmaps - Read the inode and block bitmaps from the mounted device
 * @return 0 on success, negative error code on failure
 */
static int read_bitmaps()
{
    const int ret = mounted_dev->read_sectors(mounted_dev->ctx, DISKFS_INODE_BITMAP_START,
                                DISKFS_INODE_BITMAP_SECTORS, inode_bitmap);
    if (ret != 0) return ret;

    return mounted_dev->read_sectors(mounted_dev->ctx, DISKFS_BLOCK_BITMAP_START,
                        DISKFS_BLOCK_BITMAP_SECTORS, block_bitmap);
}

/**
 * @brief write_bitmaps - Write the inode and block bitmaps to the mounted device
 * @return 0 on success, negative error code on failure
 */
static int write_bitmaps()
{
    const int ret = mounted_dev->write_sectors(mounted_dev->ctx, DISKFS_INODE_BITMAP_START,
                            DISKFS_INODE_BITMAP_SECTORS, inode_bitmap);
    if (ret != 0) return ret;

    return mounted_dev->write_sectors(mounted_dev->ctx, DISKFS_BLOCK_BITMAP_START,
                        DISKFS_BLOCK_BITMAP_SECTORS, block_bitmap);
}

/**
 * @brief read_inode - Read an inode from the mounted device
 * @param ino The inode number to read
 * @param inode The inode structure to fill with data
 * @return 0 on success, negative error code on failure
 */
static int read_inode(const uint32_t ino, struct diskfs_inode* inode)
{
    if (ino >= DISKFS_MAX_INODES)
    {
        return -1;
    }

    for (int i = 0; i < 8; i++)
    {
        if (inode_cache_valid[i] && inode_cache_num[i] == ino)
        {
            memcpy(inode, &inode_cache[i], sizeof(struct diskfs_inode));
            return 0;
        }
    }

    const uint32_t sector = DISKFS_INODE_TABLE_START + (ino / DISKFS_INODES_PER_SECTOR);
    const uint32_t offset = (ino % DISKFS_INODES_PER_SECTOR) * DISKFS_INODE_SIZE;

    uint8_t sector_buf[DISKFS_SECTOR_SIZE];
    const int ret = mounted_dev->read_sectors(mounted_dev->ctx, sector, 1, sector_buf);
    if (ret != 0)
    {
        return ret;
    }

    memcpy(inode, sector_buf + offset, sizeof(struct diskfs_inode));

    static int cache_idx = 0;
    inode_cache_num[cache_idx] = ino;
    memcpy(&inode_cache[cache_idx], inode, sizeof(struct diskfs_inode));
    inode_cache_valid[cache_idx] = 1;
    cache_idx = (cache_idx + 1) % 8;

    return 0;
}

/**
 * @brief write_inode - Write an inode to the mounted device
 * @param ino The inode number to write
 * @param inode The inode structure containing data to write
 * @return 0 on success, negative error code on failure
 */
static int write_inode(const uint32_t ino, const struct diskfs_inode* inode)
{
    if (ino >= DISKFS_MAX_INODES)
    {
        return -1;
    }

    const uint32_t sector = DISKFS_INODE_TABLE_START + (ino / DISKFS_INODES_PER_SECTOR);
    const uint32_t offset = (ino % DISKFS_INODES_PER_SECTOR) * DISKFS_INODE_SIZE;

    uint8_t sector_buf[DISKFS_SECTOR_SIZE];
    int ret = mounted_dev->read_sectors(mounted_dev->ctx, sector, 1, sector_buf);
    if (ret != 0)
    {
        return ret;
    }

    memcpy(sector_buf + offset, inode, sizeof(struct diskfs_inode));

    ret = mounted_dev->write_sectors(mounted_dev->ctx, sector, 1, sector_buf);
    if (ret != 0)
    {
        return ret;
    }

    for (int i = 0; i < 8; i++)
    {
        if (inode_cache_valid[i] && inode_cache_num[i] == ino)
        {
            memcpy(&inode_cache[i], inode, sizeof(struct diskfs_inode));
            break;
        }
    }

    return 0;
}

/**
 * @brief alloc_block - Allocate a block from the mounted device
 * @return The block number on success, negative error code on failure
 */
static int alloc_block(void)
{
    const int block = bitmap_find_free(block_bitmap, superblock.total_blocks);
    if (block < 0)
    {
        return -1;
    }

    bitmap_set(block_bitmap, (uint32_t)block);
    superblock.free_blocks--;

    return block;
}

/**
 * @brief free_block - Free a block on the mounted device
 * @param block The block number to free
 */
static void free_block(const uint32_t block)
{
    if (block >= superblock.total_blocks)
    {
        return;
    }

    bitmap_clear(block_bitmap, block);
    superblock.free_blocks++;
}

/**
 * @brief alloc_inode - Allocate an inode from the mounted device
 * @return The inode number on success, negative error code on failure
 */
static int alloc_inode(void)
{
    const int ino = bitmap_find_free(inode_bitmap, superblock.total_inodes);
    if (ino < 0)
    {
        return -1;
    }

    bitmap_set(inode_bitmap, (uint32_t)ino);
    superblock.free_inodes--;

    return ino;
}

/**
 * @brief free_inode - Free an inode on the mounted device
 * @param ino The inode number to free
 */
static void free_inode(const uint32_t ino)
{
    if (ino >= superblock.total_inodes)
    {
        return;
    }

    bitmap_clear(inode_bitmap, ino);
    superblock.free_inodes++;
}

int diskfs_format(const uint8_t drive)
{
    if (!ata_drive_exists(drive))
    {
        log_error_fmt("diskfs_format: drive does not exist (%d)", drive);
        return -1;
    }

    log_info_fmt("Formatting drive %d...", drive);

    memset(&superblock, 0, sizeof(superblock));
    superblock.magic = DISKFS_MAGIC;
    superblock.version = DISKFS_VERSION;
    superblock.total_inodes = DISKFS_MAX_INODES;
    superblock.total_blocks = DISKFS_MAX_BLOCKS;
    superblock.free_inodes = DISKFS_MAX_INODES - 1;
    superblock.free_blocks = DISKFS_MAX_BLOCKS - 1;
    superblock.root_inode = 0;
    superblock.block_size = DISKFS_BLOCK_SIZE;

    memset(inode_bitmap, 0, sizeof(inode_bitmap));
    memset(block_bitmap, 0, sizeof(block_bitmap));

    bitmap_set(inode_bitmap, 0);
    bitmap_set(block_bitmap, 0);

    if (ata_write_sectors(drive, DISKFS_SUPERBLOCK_SECTOR, 1, &superblock) != 0)
    {
        console_write("diskfs_format: failed to write superblock\n");
        log_error("diskfs_format: failed to write superblock");
        return -1;
    }

    if (ata_write_sectors(drive, DISKFS_INODE_BITMAP_START, DISKFS_INODE_BITMAP_SECTORS, inode_bitmap) != 0 ||
        ata_write_sectors(drive, DISKFS_BLOCK_BITMAP_START, DISKFS_BLOCK_BITMAP_SECTORS, block_bitmap) != 0)
    {
        console_write("diskfs_format: failed to write bitmaps\n");
        log_error("diskfs_format: failed to write bitmaps");
        return -1;
    }

    struct diskfs_inode root;
    memset(&root, 0, sizeof(root));
    root.type = DISKFS_TYPE_DIR;
    root.size = 0;
    root.parent_inode = 0;
    root.ctime = timer_get_ticks();
    root.mtime = root.ctime;

    uint8_t inode_sector[DISKFS_SECTOR_SIZE];
    memset(inode_sector, 0, sizeof(inode_sector));
    memcpy(inode_sector, &root, sizeof(root));

    if (ata_write_sectors(drive, DISKFS_INODE_TABLE_START, 1, inode_sector) != 0)
    {
        console_write("diskfs_format: failed to write root inode\n");
        log_error("diskfs_format: failed to write root inode");
        return -1;
    }

    log_info("Format complete");
    return 0;
}

int diskfs_mount_dev(const struct blockdev_ops* dev)
{
    if (!dev || !dev->read_sectors || !dev->write_sectors)
    {
        log_error("diskfs_mount_dev: invalid block device ops");
        return -1;
    }

    log_info_fmt("Mounting device '%s'...", dev->name ? dev->name : "?");

    mounted_dev_storage = *dev;
    mounted_dev = &mounted_dev_storage;

    if (read_superblock() != 0)
    {
        log_error("diskfs_mount_dev: failed to read superblock");
        mounted_dev = NULL;
        return -1;
    }

    if (superblock.magic != DISKFS_MAGIC)
    {
        log_error("diskfs_mount_dev: invalid magic number");
        mounted_dev = NULL;
        return -1;
    }

    if (superblock.version != DISKFS_VERSION)
    {
        log_error("diskfs_mount_dev: unsupported filesystem version");
        mounted_dev = NULL;
        return -1;
    }

    if (read_bitmaps() != 0)
    {
        log_error("diskfs_mount_dev: failed to read bitmaps");
        mounted_dev = NULL;
        return -1;
    }

    memset(inode_cache_valid, 0, sizeof(inode_cache_valid));

    struct diskfs_inode root;
    if (read_inode(superblock.root_inode, &root) != 0 || root.type != DISKFS_TYPE_DIR)
    {
        log_error("diskfs_mount_dev: invalid root inode");
        mounted_dev = NULL;
        return -1;
    }

    log_info_fmt("Mounted '%s' successfully (%d free inodes, %d free blocks)",
                dev->name ? dev->name : "?", superblock.free_inodes, superblock.free_blocks);

    return 0;
}

int diskfs_mount(const uint8_t drive)
{
    if (!ata_drive_exists(drive))
    {
        log_error_fmt("diskfs_mount: drive does not exist (%d)", drive);
        return -1;
    }

    const struct blockdev_ops ata_ops = {
        .read_sectors = diskfs_ata_read_sectors,
        .write_sectors = diskfs_ata_write_sectors,
        .ctx = (void*)(uintptr_t)drive,
        .name = "ata",
    };

    return diskfs_mount_dev(&ata_ops);
}

int diskfs_unmount(void)
{
    if (!mounted_dev)
    {
        return -1;
    }

    log_info("Unmounting...");
    diskfs_sync();
    mounted_dev = NULL;

    return 0;
}

int diskfs_sync(void)
{
    if (!mounted_dev)
    {
        return -1;
    }

    write_superblock();
    write_bitmaps();

    return 0;
}

int diskfs_init(const uint8_t drive)
{
    if (diskfs_mount(drive) == 0)
    {
        log_info_fmt("Existing filesystem found on drive %d", drive);
        return 0;
    }

    log_warn("No valid filesystem found, formatting...");
    if (diskfs_format(drive) != 0)
    {
        return -1;
    }

    return diskfs_mount(drive);
}

int diskfs_create(const uint32_t parent_ino, const char* name, const uint32_t type)
{
    if (!mounted_dev)
    {
        return -1;
    }

    if (strlen(name) >= DISKFS_MAX_FILENAME)
    {
        return -1;
    }

    struct diskfs_inode parent;
    if (read_inode(parent_ino, &parent) != 0)
    {
        return -1;
    }

    if (parent.type != DISKFS_TYPE_DIR)
    {
        return -1;
    }

    if (diskfs_lookup(parent_ino, name) >= 0)
    {
        return -2;
    }

    const int ino = alloc_inode();
    if (ino < 0)
    {
        return -3;
    }

    struct diskfs_inode new_inode;
    memset(&new_inode, 0, sizeof(new_inode));
    new_inode.type = type;
    new_inode.size = 0;
    new_inode.parent_inode = parent_ino;
    new_inode.ctime = timer_get_ticks();
    new_inode.mtime = new_inode.ctime;

    if (write_inode((uint32_t)ino, &new_inode) != 0)
    {
        free_inode((uint32_t)ino);
        return -1;
    }

    struct diskfs_dirent new_entry;
    new_entry.inode = (uint32_t)ino;
    strncpy(new_entry.name, name, DISKFS_MAX_FILENAME - 1);
    new_entry.name[DISKFS_MAX_FILENAME - 1] = '\0';

    const int ret = diskfs_write(parent_ino, &new_entry, parent.size, sizeof(new_entry));
    if (ret < 0)
    {
        free_inode((uint32_t)ino);
        return -1;
    }

    diskfs_sync();

    return ino;
}

int diskfs_lookup(const uint32_t dir_ino, const char* name)
{
    if (!mounted_dev)
    {
        return -1;
    }

    struct diskfs_inode dir;
    if (read_inode(dir_ino, &dir) != 0)
    {
        return -1;
    }

    if (dir.type != DISKFS_TYPE_DIR)
    {
        return -1;
    }

    const uint32_t num_entries = dir.size / sizeof(struct diskfs_dirent);

    for (uint32_t i = 0; i < num_entries; i++)
    {
        struct diskfs_dirent entry;
        const int ret = diskfs_read(dir_ino, &entry, i * sizeof(entry), sizeof(entry));
        if (ret < 0)
        {
            break;
        }

        if (entry.inode != 0 && strcmp(entry.name, name) == 0)
        {
            return (int)entry.inode;
        }
    }

    return -1;
}

int diskfs_read(const uint32_t ino, void* buffer, const uint32_t offset, uint32_t size)
{
    if (!mounted_dev)
    {
        return -1;
    }

    struct diskfs_inode inode;
    if (read_inode(ino, &inode) != 0)
    {
        return -1;
    }

    if (offset >= inode.size)
    {
        return 0;
    }

    if (offset + size > inode.size)
    {
        size = inode.size - offset;
    }

    uint32_t bytes_read = 0;
    uint8_t* buf = buffer;

    while (bytes_read < size)
    {
        const uint32_t block_idx = (offset + bytes_read) / DISKFS_BLOCK_SIZE;
        const uint32_t block_offset = (offset + bytes_read) % DISKFS_BLOCK_SIZE;
        const uint32_t to_read = (size - bytes_read < DISKFS_BLOCK_SIZE - block_offset) ?
                                 size - bytes_read : DISKFS_BLOCK_SIZE - block_offset;

        if (block_idx >= DISKFS_DIRECT_BLOCKS)
        {
            break;
        }

        if (inode.blocks[block_idx] == 0)
        {
            break;
        }

        uint8_t block_buf[DISKFS_BLOCK_SIZE];
        const uint32_t sector = DISKFS_DATA_START + inode.blocks[block_idx];

        if (mounted_dev->read_sectors(mounted_dev->ctx, sector, 1, block_buf) != 0)
        {
            return -1;
        }

        memcpy(buf + bytes_read, block_buf + block_offset, to_read);
        bytes_read += to_read;
    }

    return (int)bytes_read;
}

int diskfs_write(const uint32_t ino, const void* buffer, const uint32_t offset, uint32_t size)
{
    if (!mounted_dev)
    {
        return -1;
    }

    struct diskfs_inode inode;
    if (read_inode(ino, &inode) != 0)
    {
        return -1;
    }

    if (offset + size > DISKFS_MAX_FILE_SIZE)
    {
        size = DISKFS_MAX_FILE_SIZE - offset;
    }

    uint32_t bytes_written = 0;
    const uint8_t* buf = buffer;

    while (bytes_written < size)
    {
        const uint32_t block_idx = (offset + bytes_written) / DISKFS_BLOCK_SIZE;
        const uint32_t block_offset = (offset + bytes_written) % DISKFS_BLOCK_SIZE;
        const uint32_t to_write = (size - bytes_written < DISKFS_BLOCK_SIZE - block_offset) ?
                                  size - bytes_written : DISKFS_BLOCK_SIZE - block_offset;

        if (block_idx >= DISKFS_DIRECT_BLOCKS)
        {
            break;
        }

        if (inode.blocks[block_idx] == 0)
        {
            const int block = alloc_block();
            if (block < 0)
            {
                break;
            }
            inode.blocks[block_idx] = (uint32_t)block;
        }

        uint8_t block_buf[DISKFS_BLOCK_SIZE];
        const uint32_t sector = DISKFS_DATA_START + inode.blocks[block_idx];

        if (block_offset != 0 || to_write < DISKFS_BLOCK_SIZE)
        {
            if (mounted_dev->read_sectors(mounted_dev->ctx, sector, 1, block_buf) != 0)
            {
                return -1;
            }
        }

        memcpy(block_buf + block_offset, buf + bytes_written, to_write);

        if (mounted_dev->write_sectors(mounted_dev->ctx, sector, 1, block_buf) != 0)
        {
            return -1;
        }

        bytes_written += to_write;
    }

    if (offset + bytes_written > inode.size)
    {
        inode.size = offset + bytes_written;
    }

    inode.mtime = timer_get_ticks();
    write_inode(ino, &inode);

    return (int)bytes_written;
}

int diskfs_truncate(const uint32_t ino)
{
    if (!mounted_dev)
    {
        return -1;
    }

    struct diskfs_inode inode;
    if (read_inode(ino, &inode) != 0)
    {
        return -1;
    }

    if (inode.type != DISKFS_TYPE_FILE)
    {
        return -1;
    }

    for (uint32_t i = 0; i < DISKFS_DIRECT_BLOCKS; i++)
    {
        if (inode.blocks[i] != 0)
        {
            free_block(inode.blocks[i]);
            inode.blocks[i] = 0;
        }
    }

    inode.size = 0;
    inode.mtime = timer_get_ticks();

    if (write_inode(ino, &inode) != 0)
    {
        return -1;
    }

    diskfs_sync();
    return 0;
}

int diskfs_readdir(const uint32_t dir_ino, struct diskfs_dirent* entries, const uint32_t max_entries)
{
    if (!mounted_dev)
    {
        return -1;
    }

    struct diskfs_inode dir;
    if (read_inode(dir_ino, &dir) != 0)
    {
        return -1;
    }

    if (dir.type != DISKFS_TYPE_DIR)
    {
        return -1;
    }

    const uint32_t num_entries = dir.size / sizeof(struct diskfs_dirent);
    const uint32_t to_read = (num_entries < max_entries) ? num_entries : max_entries;

    for (uint32_t i = 0; i < to_read; i++)
    {
        const int ret = diskfs_read(dir_ino, &entries[i], i * sizeof(struct diskfs_dirent),
                                   sizeof(struct diskfs_dirent));
        if (ret < 0)
        {
            return -1;
        }
    }

    return (int)to_read;
}

int diskfs_stat(const uint32_t ino, struct diskfs_inode* inode)
{
    if (!mounted_dev)
    {
        return -1;
    }

    return read_inode(ino, inode);
}

int diskfs_delete(const uint32_t parent_ino, const char* name)
{
    if (!mounted_dev)
    {
        return -1;
    }

    const int ino = diskfs_lookup(parent_ino, name);
    if (ino < 0)
    {
        return -1;
    }

    struct diskfs_inode inode;
    if (read_inode((uint32_t)ino, &inode) != 0)
    {
        return -1;
    }

    if (inode.type == DISKFS_TYPE_DIR && inode.size > 0)
    {
        return -2;
    }

    for (uint32_t i = 0; i < DISKFS_DIRECT_BLOCKS; i++)
    {
        if (inode.blocks[i] != 0)
        {
            free_block(inode.blocks[i]);
        }
    }

    free_inode((uint32_t)ino);

    struct diskfs_inode parent;
    if (read_inode(parent_ino, &parent) != 0)
    {
        return -1;
    }

    const uint32_t num_entries = parent.size / sizeof(struct diskfs_dirent);

    for (uint32_t i = 0; i < num_entries; i++)
    {
        struct diskfs_dirent entry;
        diskfs_read(parent_ino, &entry, i * sizeof(entry), sizeof(entry));

        if (entry.inode == (uint32_t)ino)
        {
            entry.inode = 0;
            diskfs_write(parent_ino, &entry, i * sizeof(entry), sizeof(entry));
            break;
        }
    }

    diskfs_sync();

    return 0;
}

int diskfs_is_mounted(void)
{
    return mounted_dev != NULL;
}
