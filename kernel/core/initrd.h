#ifndef INITRD_H
#define INITRD_H

#include "include/types.h"

extern const uint8_t _binary_init_elf_start[];
extern const uint8_t _binary_init_elf_end[];
extern const uint8_t _binary_echo_elf_start[];
extern const uint8_t _binary_echo_elf_end[];
extern const uint8_t _binary_cat_elf_start[];
extern const uint8_t _binary_cat_elf_end[];
extern const uint8_t _binary_ls_elf_start[];
extern const uint8_t _binary_ls_elf_end[];
extern const uint8_t _binary_sh_elf_start[];
extern const uint8_t _binary_sh_elf_end[];

#define INITRD_INIT_PATH "/bin/init"

struct initrd_file
{
    const char* path;
    const uint8_t* data;
    size_t size;
};

/**
 * @brief Return the number of files embedded in the initrd.
 */
size_t initrd_file_count(void);

/**
 * @brief Copy an embedded initrd file descriptor into out.
 */
int initrd_get_file(size_t index, struct initrd_file* out);

/**
 * @brief Install embedded user programs into the active VFS.
 */
int initrd_install(void);

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
