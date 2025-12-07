#ifndef INITRD_H
#define INITRD_H

#include "../include/types.h"

/**
 * @brief Start of the embedded init binary
 *
 * Symbol provided by objcopy when embedding the binary.
 */
extern const uint8_t _binary_init_elf_start[];

/**
 * @brief End of the embedded init binary
 *
 * Symbol provided by objcopy when embedding the binary.
 */
extern const uint8_t _binary_init_elf_end[];

/**
 * @brief Size of the embedded init binary
 *
 * Symbol provided by objcopy when embedding the binary.
 */
extern const uint8_t _binary_init_elf_size[];

/**
 * @brief Get pointer to the embedded init binary
 * @return Pointer to the start of the init.elf data
 */
static inline const void* initrd_get_init(void)
{
    return (const void*)_binary_init_elf_start;
}

/**
 * @brief Get size of the embedded init binary
 * @return Size in bytes
 */
static inline size_t initrd_get_init_size(void)
{
    return (size_t)(_binary_init_elf_end - _binary_init_elf_start);
}

#endif
