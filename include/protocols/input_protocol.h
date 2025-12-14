/**
 * @file input_protocol.h
 * @brief Input server IPC protocol definitions
 *
 * This header defines the message types and structures for communication
 * with the input server in the mexOS microkernel architecture.
 */

#ifndef INCLUDE_INPUT_PROTOCOL_H
#define INCLUDE_INPUT_PROTOCOL_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Input server port name for nameserver registration
 */
#define INPUT_SERVER_PORT_NAME "input"

/**
 * @brief Maximum input events in a single message
 */
#define INPUT_MAX_EVENTS 16

/**
 * @brief Input message types \enum input_msg_type
 */
enum input_msg_type
{
    INPUT_MSG_REGISTER = 0x0300,
    INPUT_MSG_UNREGISTER = 0x0301,
    INPUT_MSG_POLL = 0x0302,
    INPUT_MSG_READ = 0x0303,
    INPUT_MSG_EVENT = 0x0310,
    INPUT_MSG_RESPONSE = 0x03FF
};

/**
 * @brief Input event types \enum input_event_type
 */
enum input_event_type
{
    INPUT_EVENT_KEY_PRESS = 0x01,
    INPUT_EVENT_KEY_RELEASE = 0x02,
    INPUT_EVENT_MOUSE_MOVE = 0x10,
    INPUT_EVENT_MOUSE_BTN = 0x11
};

/**
 * @brief Key modifier flags \enum input_key_modifier
 */
enum input_key_modifier
{
    INPUT_MOD_SHIFT = 0x01,
    INPUT_MOD_CTRL = 0x02,
    INPUT_MOD_ALT = 0x04,
    INPUT_MOD_CAPS = 0x08
};

/**
 * @brief Input event structure \struct input_event
 */
struct input_event
{
    uint8_t type;
    uint8_t modifiers;
    uint16_t scancode;
    uint8_t keychar;
    uint8_t reserved;
    int16_t mouse_x;
    int16_t mouse_y;
    uint8_t mouse_btn;
    uint8_t padding;
};

/**
 * @brief Input register request structure \struct input_register_request
 */
struct input_register_request
{
    uint32_t event_mask;
    int32_t port_id;
};

/**
 * @brief Input read response structure \struct input_read_response
 */
struct input_read_response
{
    int32_t status;
    uint8_t event_count;
    uint8_t reserved[3];
    struct input_event events[INPUT_MAX_EVENTS];
};

/**
 * @brief Input poll response structure \struct input_poll_response
 */
struct input_poll_response
{
    int32_t status;
    uint32_t events_pending;
};

/**
 * @brief Generic input response structure \struct input_response
 */
struct input_response
{
    int32_t status;
};


#ifdef __cplusplus
}
#endif

#endif // INCLUDE_INPUT_PROTOCOL_H