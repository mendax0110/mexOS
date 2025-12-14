#ifndef INCLUDE_CONSOLE_PROTOCOL_H
#define INCLUDE_CONSOLE_PROTOCOL_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Console server port name for nameserver registration
 */
#define CONSOLE_SERVER_PORT_NAME "console"

/**
 * @brief Maximum write buffer size in a single message
 */
#define CONSOLE_MAX_WRITE_SIZE 240

/**
 * @brief Console message types \enum console_msg_type
 */
enum console_msg_type
{
    CONSOLE_MSG_WRITE = 0x0200,
    CONSOLE_MSG_CLEAR = 0x0201,
    CONSOLE_MSG_SET_COLOR = 0x0202,
    CONSOLE_MSG_GET_SIZE = 0x0203,
    CONSOLE_MSG_SET_POS = 0x0204,
    CONSOLE_MSG_GET_POS = 0x0205,
    CONSOLE_MSG_SCROLL = 0x0206,
    CONSOLE_MSG_VTERM_CREATE = 0x0210,
    CONSOLE_MSG_VTERM_SWITCH = 0x0211,
    CONSOLE_MSG_RESPONSE = 0x02FF
};

/**
 * @brief VGA color codes \enum console_color
 */
enum console_color
{
    CONSOLE_COLOR_BLACK = 0,
    CONSOLE_COLOR_BLUE = 1,
    CONSOLE_COLOR_GREEN = 2,
    CONSOLE_COLOR_CYAN = 3,
    CONSOLE_COLOR_RED = 4,
    CONSOLE_COLOR_MAGENTA = 5,
    CONSOLE_COLOR_BROWN = 6,
    CONSOLE_COLOR_LIGHT_GREY = 7,
    CONSOLE_COLOR_DARK_GREY = 8,
    CONSOLE_COLOR_LIGHT_BLUE = 9,
    CONSOLE_COLOR_LIGHT_GREEN = 10,
    CONSOLE_COLOR_LIGHT_CYAN = 11,
    CONSOLE_COLOR_LIGHT_RED = 12,
    CONSOLE_COLOR_LIGHT_MAGENTA = 13,
    CONSOLE_COLOR_YELLOW = 14,
    CONSOLE_COLOR_WHITE = 15
};

/**
 * @brief Console write request structure \struct console_write_request
 */
struct console_write_request
{
    uint8_t length;
    char data[CONSOLE_MAX_WRITE_SIZE];
};

/**
 * @brief Console set color request structure \struct console_set_color_request
 */
struct console_set_color_request
{
    uint8_t foreground;
    uint8_t background;
};

/**
 * @brief Console cursor position structure \struct console_position
 */
struct console_position
{
    uint16_t x;
    uint16_t y;
};

/**
 * @brief Console size response structure \struct console_size_response
 */
struct console_size_response
{
    int32_t status;
    uint16_t width;
    uint16_t height;
};

/**
 * @brief Console scroll request structure \struct console_scroll_request
 */
struct console_scroll_request
{
    int16_t lines;
};

/**
 * @brief Virtual terminal create response structure \struct console_vterm_response
 */
struct console_vterm_response
{
    int32_t status;
    int32_t term_id;
};

/**
 * @brief Generic console response structure \struct console_response
 */
struct console_response
{
    int32_t status;
};

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_CONSOLE_PROTOCOL_H