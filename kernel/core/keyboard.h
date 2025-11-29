#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_BUFFER_SIZE  256

void keyboard_init(void);
char keyboard_getchar(void);
int keyboard_has_data(void);

#ifdef __cplusplus
}
#endif

#endif
