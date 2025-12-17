#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include "../include/types.h"
#include "../mm/vmm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ELF magic number bytes
 */
#define ELF_MAGIC0      0x7F
#define ELF_MAGIC1      'E'
#define ELF_MAGIC2      'L'
#define ELF_MAGIC3      'F'

/**
 * @brief ELF class types
 */
#define ELFCLASS32      1
#define ELFCLASS64      2

/**
 * @brief ELF data encoding
 */
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

/**
 * @brief ELF file types
 */
#define ET_NONE         0
#define ET_REL          1
#define ET_EXEC         2
#define ET_DYN          3
#define ET_CORE         4

/**
 * @brief ELF machine types
 */
#define EM_386          3
#define EM_X86_64       62

/**
 * @brief ELF program header types
 */
#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6

/**
 * @brief ELF program header flags
 */
#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4

/**
 * @brief ELF32 file header structure
 */
struct elf32_header
{
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} PACKED;

/**
 * @brief ELF32 program header structure
 */
struct elf32_phdr
{
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} PACKED;

/**
 * @brief ELF32 section header structure
 */
struct elf32_shdr
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} PACKED;

/**
 * @brief ELF load result structure
 */
struct elf_load_result
{
    uint32_t entry_point;
    uint32_t brk;
};

/**
 * @brief Validate an ELF32 header
 * @param header Pointer to the ELF header
 * @return 0 on success, -1 on failure
 */
int elf_validate(const struct elf32_header* header);

/**
 * @brief Load an ELF32 executable into an address space
 * @param data Pointer to the ELF file data in memory
 * @param size Size of the ELF file data
 * @param page_dir Page directory to load into
 * @param result Pointer to store load results (entry point, brk)
 * @return 0 on success, negative error code on failure
 */
int elf_load(const void* data, size_t size, page_directory_t* page_dir, struct elf_load_result* result);

/**
 * @brief Load an ELF32 executable from the filesystem
 * @param path Path to the ELF file
 * @param page_dir Page directory to load into
 * @param result Pointer to store load results
 * @return 0 on success, negative error code on failure
 */
int elf_load_file(const char* path, page_directory_t* page_dir, struct elf_load_result* result);

/**
 * @brief Enter user mode at the specified entry point and stack
 * @param entry Entry point address
 * @param user_stack User stack pointer
 * @param pd Page directory for user mode
 */
__attribute__((unused)) void enter_user_mode(uint32_t entry, uint32_t user_stack, page_directory_t* pd);

#ifdef __cplusplus
}
#endif

#endif
