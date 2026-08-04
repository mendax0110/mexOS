#ifndef KERNEL_H
#define KERNEL_H

#include "../shared/types.h"

/**
 * @brief Entry point for the kernel
 * @param mboot_magic Multiboot magic number
 * @param mboot_info Pointer to multiboot information structure
 */
void kernel_main(uint32_t mboot_magic, uint32_t mboot_info);

#endif
