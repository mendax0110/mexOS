#ifndef KERNEL_INITRD_H
#define KERNEL_INITRD_H

#include "../include/types.h"

/**
 * @brief Entry in the initrd \struct initrd_entry
 */
struct initrd_entry
{
    const char* name;
    const void* data;
    size_t size;
};

/**
 * @brief Get number of entries in initrd
 * @return Number of entries
 */
size_t initrd_num_entries(void);

/**
 * @brief Get entry by index
 * @param idx Index of the entry
 * @return Pointer to the entry, or NULL if out of bounds
 */
const struct initrd_entry* initrd_get_entry(size_t idx);

/**
 * @brief Search for an entry by name
 * @param name Name of the entry to find
 * @return Pointer to the entry, or NULL if not found
 */
const struct initrd_entry* initrd_find(const char* name);

/**
 * @brief Initialize initrd entries (set sizes at runtime)
 */
void initrd_entries_init(void);

/**
 * @brief init.elf
 */
extern const uint8_t _binary_init_elf_start[];
extern const uint8_t _binary_init_elf_end[];

/**
 * @brief console.elf
 */
extern const uint8_t _binary_shell_elf_start[];
extern const uint8_t _binary_shell_elf_end[];

/**
 * @brief console.elf
 */
extern const uint8_t _binary_console_elf_start[];
extern const uint8_t _binary_console_elf_end[];

/**
 * @brief input.elf
 */
extern const uint8_t _binary_input_elf_start[];
extern const uint8_t _binary_input_elf_end[];

/**
 * @brief vfs.elf
 */
extern const uint8_t _binary_vfs_elf_start[];
extern const uint8_t _binary_vfs_elf_end[];

/**
 * @brief block.elf
 */
extern const uint8_t _binary_block_elf_start[];
extern const uint8_t _binary_block_elf_end[];

/**
 * @brief devmgr.elf
 */
extern const uint8_t _binary_devmgr_elf_start[];
extern const uint8_t _binary_devmgr_elf_end[];

/**
 * @brief Get pointer to the embedded init binary
 */
static inline const void* initrd_get_init(void)
{
    return (const void*)_binary_init_elf_start;
}

/**
 * @brief Get size of the embedded init binary
 */
static inline size_t initrd_get_init_size(void)
{
    return (size_t)(_binary_init_elf_end - _binary_init_elf_start);
}

#endif // KERNEL_INITRD_H
