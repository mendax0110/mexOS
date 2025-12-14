#ifndef INCLUDE_BLOCK_PROTOCOL_H
#define INCLUDE_BLOCK_PROTOCOL_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Block device server port name for nameserver registration
 */
#define BLOCK_SERVER_PORT_NAME "block"

/**
 * @brief Block device message types \enum block_msg_type
 */
enum block_msg_type
{
    BLOCK_MSG_READ = 0x0100,
    BLOCK_MSG_WRITE = 0x0101,
    BLOCK_MSG_GET_INFO = 0x0102,
    BLOCK_MSG_FLUSH = 0x0103,
    BLOCK_MSG_RESPONSE = 0x01FF
};

/**
 * @brief Block device types \enum block_device_type
 */
enum block_device_type
{
    BLOCK_DEV_NONE = 0,
    BLOCK_DEV_ATA = 1,
    BLOCK_DEV_AHCI = 2,
    BLOCK_DEV_NVME = 3
};

/**
 * @brief Block read request structure \struct block_read_request
 */
struct block_read_request
{
    uint8_t device_id;
    uint8_t reserved[3];
    uint32_t lba;
    uint32_t count;
    uint32_t buffer_addr;
};

/**
 * @brief Block write request structure \struct block_write_request
 */
struct block_write_request
{
    uint8_t device_id;
    uint8_t reserved[3];
    uint32_t lba;
    uint32_t count;
    uint32_t buffer_addr;
};

/**
 * @brief Block device info request structure \struct block_info_request
 */
struct block_info_request
{
    uint8_t device_id;
};

/**
 * @brief Block device info response structure \struct block_info_response
 */
struct block_info_response
{
    int32_t status;
    uint8_t device_type;
    uint8_t reserved[3];
    uint32_t sector_size;
    uint32_t sector_count;
    char model[40];
};

/**
 * @brief Block operation response structure \struct block_response
 */
struct block_response
{
    int32_t status;
    uint32_t bytes_transferred;
};

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_BLOCK_PROTOCOL_H */