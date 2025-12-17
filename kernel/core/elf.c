#include "elf.h"
#include "../../servers/vfs/fs.h"
#include "../../shared/log.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../../shared/string.h"
#include "../include/cast.h"

int elf_validate(const struct elf32_header* header)
{
    if (!header)
    {
        log_warn("elf_validate: null header");
        return -1;
    }

    if (header->e_ident[0] != ELF_MAGIC0 ||
        header->e_ident[1] != ELF_MAGIC1 ||
        header->e_ident[2] != ELF_MAGIC2 ||
        header->e_ident[3] != ELF_MAGIC3)
    {
        log_warn("elf_validate: invalid ELF magic number");
        return -1;
    }

    if (header->e_ident[4] != ELFCLASS32)
    {
        log_warn("elf_validate: unsupported ELF class (not 32-bit)");
        return -1;
    }

    if (header->e_ident[5] != ELFDATA2LSB)
    {
        log_warn("elf_validate: unsupported ELF data encoding (not little-endian)");
        return -1;
    }

    if (header->e_type != ET_EXEC)
    {
        log_warn("elf_validate: unsupported ELF type (not executable)");
        return -1;
    }

    if (header->e_machine != EM_386)
    {
        log_warn("elf_validate: unsupported ELF machine (not i386)");
        return -1;
    }

    return 0;
}

int elf_load(const void* data, size_t size, page_directory_t* page_dir, struct elf_load_result* result)
{
    if (!data || !page_dir || !result)
    {
        log_warn("elf_load: invalid arguments");
        return -1;
    }

    if (size < sizeof(struct elf32_header))
    {
        log_warn("elf_load: data size too small for ELF header");
        return -1;
    }

    const struct elf32_header* header = (const struct elf32_header*)data;

    if (elf_validate(header) != 0)
    {
        log_warn_fmt("elf_load: ELF validation failed: entry 0x%X", header->e_entry);
        return -1;
    }

    if (header->e_phoff == 0 || header->e_phnum == 0)
    {
        log_warn("elf_load: no program headers found");
        return -1;
    }

    if (header->e_phoff + header->e_phnum * sizeof(struct elf32_phdr) > size)
    {
        log_warn("elf_load: program headers exceed ELF data size");
        return -1;
    }

    page_directory_t* old_dir = vmm_get_current_directory();
    vmm_switch_address_space(page_dir);

    const struct elf32_phdr* phdrs = (const struct elf32_phdr*)((const uint8_t*)data + header->e_phoff);

    result->entry_point = header->e_entry;
    result->brk = 0;

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const struct elf32_phdr* phdr = &phdrs[i];

        if (phdr->p_type != PT_LOAD || phdr->p_memsz == 0)
        {
            continue;
        }

        if (phdr->p_vaddr >= KERNEL_VIRTUAL_BASE)
        {
            log_warn_fmt("elf_load: segment at 0x%X in kernel space", phdr->p_vaddr);
            vmm_switch_address_space(old_dir);
            return -1;
        }

        uint32_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdr->p_flags & PF_W)
        {
            flags |= PAGE_WRITE;
        }

        uint32_t vaddr     = phdr->p_vaddr & ~0xFFF;
        uint32_t vaddr_end = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & ~0xFFF;

        for (uint32_t page = vaddr; page < vaddr_end; page += PAGE_SIZE)
        {
            if (!vmm_is_mapped(page_dir, page))
            {
                if (vmm_alloc_page(page_dir, page, flags) != 0)
                {
                    log_warn_fmt("elf_load: failed to map page 0x%X", page);
                    vmm_switch_address_space(old_dir);
                    return -1;
                }
            }
        }

        if (phdr->p_filesz)
        {
            if (phdr->p_offset + phdr->p_filesz > size)
            {
                log_warn("elf_load: segment exceeds ELF size");
                vmm_switch_address_space(old_dir);
                return -1;
            }

            memcpy(PTR_FROM_U32(phdr->p_vaddr), (const uint8_t*)data + phdr->p_offset, phdr->p_filesz);
        }

        if (phdr->p_memsz > phdr->p_filesz)
        {
            memset(PTR_FROM_U32(phdr->p_vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
        }

        uint32_t end = phdr->p_vaddr + phdr->p_memsz;
        if (end > result->brk)
            result->brk = end;
    }

    result->brk = (result->brk + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    /* Restore kernel address space */
    vmm_switch_address_space(old_dir);

    return 0;
}

int elf_load_file(const char* path, page_directory_t* page_dir, struct elf_load_result* result)
{
    if (!path || !page_dir || !result)
    {
        log_warn("elf_load_file: invalid arguments");
        return -1;
    }

    static uint8_t file_buffer[FS_MAX_FILE_SIZE];

    const int bytes_read = fs_read(path, (char*)file_buffer, FS_MAX_FILE_SIZE);
    if (bytes_read < 0)
    {
        log_warn_fmt("elf_load_file: failed to read file '%s'", path);
        return -1;
    }

    return elf_load(file_buffer, (size_t)bytes_read, page_dir, result);
}

__attribute__((unused)) __attribute__((noreturn))
void enter_user_mode(uint32_t entry, uint32_t user_stack,
                     page_directory_t* pd)
{
    vmm_switch_address_space(pd);

    asm volatile (
            "cli\n"
            "mov $0x23, %%ax\n"
            "mov %%ax, %%ds\n"
            "mov %%ax, %%es\n"
            "mov %%ax, %%fs\n"
            "mov %%ax, %%gs\n"

            "pushl $0x23\n"      // SS
            "pushl %0\n"         // ESP
            "pushl $0x202\n"     // EFLAGS (IF=1)
            "pushl $0x1B\n"      // CS
            "pushl %1\n"         // EIP
            "iret\n"
            :
            : "r"(user_stack), "r"(entry)
            : "memory"
            );

    __builtin_unreachable();
}