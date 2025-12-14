#ifndef SERVER_BLOCK_CLIENT_H
#define SERVER_BLOCK_CLIENT_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize block client
 * @return 0 on success, negative error code on failure
 */
int block_client_init(void);

/**
 * @brief Check if a drive exists
 * @param drive Drive number
 * @return 1 if exists, 0 if not
 */
int block_drive_exists(uint8_t drive);

/**
 * @brief Read sectors from block device
 * @param drive Drive number
 * @param lba Starting logical block address
 * @param count Number of sectors to read
 * @param buffer Buffer to store data
 * @return 0 on success, negative error code on failure
 */
int block_read_sectors(uint8_t drive, uint32_t lba, uint8_t count, void* buffer);

/**
 * @brief Write sectors to block device
 * @param drive Drive number
 * @param lba Starting logical block address
 * @param count Number of sectors to write
 * @param buffer Data to write
 * @return 0 on success, negative error code on failure
 */
int block_write_sectors(uint8_t drive, uint32_t lba, uint8_t count, const void* buffer);

/**
 * @brief Get block device info
 * @param drive Drive number
 * @param sector_size Output: sector size
 * @param sector_count Output: total sector count
 * @return 0 on success, negative error code on failure
 */
int block_get_info(uint8_t drive, uint32_t* sector_size, uint32_t* sector_count);

#ifdef __cplusplus
}
#endif

#endif
