#ifndef SERVERS_CONSOLE_PROTOCOL_H
#define SERVERS_CONSOLE_PROTOCOL_H

#include "../../include/protocols/console_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VGA text mode dimensions
 */
#define CONSOLE_VGA_WIDTH  80
#define CONSOLE_VGA_HEIGHT 25

/**
 * @brief VGA text mode memory address
 */
#define CONSOLE_VGA_MEMORY 0xB8000

/**
 * @brief Maximum number of virtual terminals
 */
#define CONSOLE_MAX_VTERMS 8

/**
 * @brief Virtual terminal buffer size
 */
#define CONSOLE_VTERM_BUFFER_SIZE (CONSOLE_VGA_WIDTH * CONSOLE_VGA_HEIGHT * 2)

/**
 * @brief Internal virtual terminal structure \struct console_vterm
 */
struct console_vterm
{
    uint8_t id;
    uint8_t active;
    uint8_t fg_color;
    uint8_t bg_color;
    uint16_t cursor_x;
    uint16_t cursor_y;
    pid_t owner_pid;
    uint8_t buffer[CONSOLE_VTERM_BUFFER_SIZE];
};

#ifdef __cplusplus
}
#endif

#endif // SERVERS_CONSOLE_PROTOCOL_H