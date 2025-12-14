#ifndef SERVER_INPUT_PROTOCOL_H
#define SERVER_INPUT_PROTOCOL_H

#include "../../include/protocols/input_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Keyboard controller I/O ports
 */
#define INPUT_KB_DATA_PORT    0x60
#define INPUT_KB_STATUS_PORT  0x64
#define INPUT_KB_COMMAND_PORT 0x64

/**
 * @brief Keyboard status flags
 */
#define INPUT_KB_STATUS_OUTPUT_FULL 0x01
#define INPUT_KB_STATUS_INPUT_FULL  0x02

/**
 * @brief Maximum registered input clients
 */
#define INPUT_MAX_CLIENTS 16

/**
 * @brief Input event queue size
 */
#define INPUT_QUEUE_SIZE 64

/**
 * @brief Input client registration structure \struct input_client
 */
struct input_client
{
    pid_t pid;
    int32_t port_id;
    uint32_t event_mask;
    uint8_t active;
    uint8_t reserved[3];
};

/**
 * @brief Internal input state \struct input_state
 */
struct input_state
{
    uint8_t shift_pressed;
    uint8_t ctrl_pressed;
    uint8_t alt_pressed;
    uint8_t caps_lock;
    uint8_t num_lock;
    uint8_t scroll_lock;
    uint8_t extended_key;
    uint8_t reserved;
};

#ifdef __cplusplus
}
#endif

#endif // SERVER_INPUT_PROTOCOL_H