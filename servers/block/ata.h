#ifndef KERNEL_ATA_H
#define KERNEL_ATA_H

#include "include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ATA port addresses
 */
#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_IO    0x170
#define ATA_SECONDARY_CTRL  0x376

/**
 * @brief ATA registers (offset from base I/O port)
 */
#define ATA_REG_DATA        0
#define ATA_REG_ERROR       1
#define ATA_REG_FEATURES    1
#define ATA_REG_SECCOUNT    2
#define ATA_REG_LBA_LO      3
#define ATA_REG_LBA_MID     4
#define ATA_REG_LBA_HI      5
#define ATA_REG_DRIVE       6
#define ATA_REG_STATUS      7
#define ATA_REG_COMMAND     7

/**
 * @brief ATA status register bits
 */
#define ATA_SR_BSY          0x80  // Busy
#define ATA_SR_DRDY         0x40  // Drive ready
#define ATA_SR_DF           0x20  // Drive write fault
#define ATA_SR_DSC          0x10  // Drive seek complete
#define ATA_SR_DRQ          0x08  // Data request ready
#define ATA_SR_CORR         0x04  // Corrected data
#define ATA_SR_IDX          0x02  // Index
#define ATA_SR_ERR          0x01  // Error

/**
 * @brief ATA error register bits
 */
#define ATA_ER_BBK          0x80  // Bad block
#define ATA_ER_UNC          0x40  // Uncorrectable data
#define ATA_ER_MC           0x20  // Media changed
#define ATA_ER_IDNF         0x10  // ID mark not found
#define ATA_ER_MCR          0x08  // Media change request
#define ATA_ER_ABRT         0x04  // Command aborted
#define ATA_ER_TK0NF        0x02  // Track 0 not found
#define ATA_ER_AMNF         0x01  // Address mark not found

/**
 * @brief ATA commands
 */
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_READ_PIO_EXT 0x24
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_IDENTIFY    0xEC

/**
 * @brief ATA drive selection
 */
#define ATA_MASTER          0
#define ATA_SLAVE           1

/**
 * @brief Sector size (512 bytes)
 */
#define ATA_SECTOR_SIZE     512

/**
 * @brief Initialize the ATA driver
 * Detects and initializes ATA drives on primary and secondary channels
 * @return 0 on success, -1 on failure
 */
int ata_init(void);

/**
 * @brief Read sectors from an ATA drive
 * @param drive Drive number (0 = primary master, 1 = primary slave, etc.)
 * @param lba Logical Block Address to read from
 * @param sector_count Number of sectors to read
 * @param buffer Buffer to store read data (must be at least sector_count * 512 bytes)
 * @return 0 on success, negative error code on failure
 */
int ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, void* buffer);

/**
 * @brief Write sectors to an ATA drive
 * @param drive Drive number (0 = primary master, 1 = primary slave, etc.)
 * @param lba Logical Block Address to write to
 * @param sector_count Number of sectors to write
 * @param buffer Buffer containing data to write (must be at least sector_count * 512 bytes)
 * @return 0 on success, negative error code on failure
 */
int ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, const void* buffer);

/**
 * @brief Check if a drive exists
 * @param drive Drive number
 * @return true if drive exists, false otherwise
 */
bool ata_drive_exists(uint8_t drive);

/**
 * @brief Get drive size in sectors
 * @param drive Drive number
 * @return Number of sectors, or 0 if drive doesn't exist
 */
uint32_t ata_get_drive_size(uint8_t drive);

#ifdef __cplusplus
}
#endif

#endif