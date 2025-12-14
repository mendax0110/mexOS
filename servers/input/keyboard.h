#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H

#include "include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
