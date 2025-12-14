#ifndef SERVERS_BLOCK_PROTOCOL_H
#define SERVERS_BLOCK_PROTOCOL_H

#include "../../include/protocols/block_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of block devices
 */
#define BLOCK_MAX_DEVICES 8

/**
 * @brief Sector size in bytes
 */
#define BLOCK_SECTOR_SIZE 512

/**
 * @brief Block device state \enum block_device_state
 */
enum block_device_state
{
    BLOCK_STATE_OFFLINE = 0,
    BLOCK_STATE_ONLINE = 1,
    BLOCK_STATE_ERROR = 2
};

/**
 * @brief Internal block device structure \struct block_device
 */
struct block_device
{
    uint8_t id;
    uint8_t type;
    uint8_t state;
    uint8_t reserved;
    uint32_t sector_size;
    uint32_t sector_count;
    char model[40];
};

#ifdef __cplusplus
}
#endif

#endif // SERVERS_BLOCK_PROTOCOL_H