#ifndef KERNEL_BLOCKDEV_H
#define KERNEL_BLOCKDEV_H

#include "../shared/types.h"

/**
 * @brief Generic interface for blockdev operations (ATA, USB etc) \struct blockdev_ops
 */
struct blockdev_ops
{
    /**
     * @brief Reads sectors from the block device
     * @param ctx The context for the block device
     * @param lba The logical block address
     * @param sector_count The number of sectors to read
     * @param buffer The buffer to store the read data
     * @return 0 on success, -1 on failure
     */
    int (*read_sectors)(void* ctx, uint32_t lba, uint8_t sector_count, void* buffer);

    /**
     * @brief Writes sectors to the block device
     * @param ctx The context for the block device
     * @param lba The logical block address
     * @param sector_count The number of sectors to write
     * @param buffer The buffer containing the data to write
     * @return 0 on success, -1 on failure
     */
    int (*write_sectors)(void* ctx, uint32_t lba, uint8_t sector_count, void* buffer);

    /**
     * @brief The context for the block device
     */
    void* ctx;

    /**
     * @brief The name of the block device
     */
    const char* name;
};

#endif // KERNEL_BLOCKDEV_H