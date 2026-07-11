#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include "../../shared/types.h"
#include "mm/vmm.h"

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

struct elf32_sym
{
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
} PACKED;

#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define ELF32_ST_TYPE(info) ((info) & 0x0F)
#define STT_FUNC 2

/**
 * @brief ELF load result structure \struct elf_load_result
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
 * @brief Load an ELF executable and prepare its initial user stack
 * @param path Path to the executable
 * @param page_dir Target address space
 * @param argc Argument count
 * @param argv Argument vector
 * @param result Load metadata
 * @param out_user_stack_base Returned user stack base address
 * @param out_user_stack_top Returned initial user stack pointer
 * @return 0 on success, negative error code on failure
 */
int elf_load_program(const char* path, page_directory_t* page_dir, int argc, const char* const argv[],
                     struct elf_load_result* result, uint32_t* out_user_stack_base, uint32_t* out_user_stack_top);

/**
 * @brief Locate the kernel's own .symtab/.strtab via Multiboot ELF section info
 * @param mboot_info Physical address of the multiboot info struct
 */
void elf_init_symbols(uint32_t mboot_info);

/**
 * @brief Get a raw pointer to the kernel's symbol table string table
 * @return The string table data, or NULL if not found
 */
char* elf_find_symtab(void);

/**
 * @brief Look up a symbol name by its address in the kernel's symbol table
 * @param addr The address to look up
 * @return A pointer to the symbol name, or NULL if not found
 */
char* elf_lookup_symbol(uint32_t addr);

/**
 * @brief Reserve memory regions for GRUB's ELF sections to prevent them from being overwritten
 * @param mboot_info Physical address of the multiboot info struct
 */
void elf_reserve_grub_sections(uint32_t mboot_info);

#ifdef __cplusplus
}
#endif

#endif
