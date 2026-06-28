#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H

#include "include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Keyboard I/O ports and buffer size
 */
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_BUFFER_SIZE  256

/*+
 * @brief Special key codes for navigating
 */
#define KEY_ARROW_UP 0x80
#define KEY_ARROW_DOWN 0x81
#define KEY_ARROW_LEFT 0x82
#define KEY_ARROW_RIGHT 0x83
#define KEY_HOME 0x84
#define KEY_END 0x85
#define KEY_LEFT_SHIFT 0x2A
#define KEY_RIGHT_SHIFT 0x36
#define KEY_LEFT_SHIFT_RELEASE 0xAA
#define KEY_RIGHT_SHIFT_RELEASE 0xB6
#define KEY_EXTENDED 0xE0
#define KEY_RELEASE_MASK 0x80
#define KEYBOARD_DISABLE 0xAD

/**
 * @brief Initialize the keyboard driver
 */
void keyboard_init(void);

/**
 * @brief Get a character from the keyboard buffer
 * @return The character, or 0 if no data is available
 */
unsigned char keyboard_getchar(void);

/**
 * @brief Check if there is data available in the keyboard buffer
 * @return Non-zero if data is available, zero otherwise
 */
int keyboard_has_data(void);

/**
 * @brief Shutdown the keyboard driver
 */
void keyboard_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
