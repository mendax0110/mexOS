#ifndef KERNEL_H
#define KERNEL_H

#include "include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Entry point for the kernel
 */
void kernel_main(void);

/**
 * @brief Handle a kernel panic
 * @param msg The panic message
 */
void kernel_panic(const char* msg);

#ifdef __cplusplus
}
#endif

#endif
