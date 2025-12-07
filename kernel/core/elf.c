#include "elf.h"
#include "fs.h"
#include "log.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../include/string.h"

int elf_validate(const struct elf32_header* header)
{
    if (!header)
    {
        return -1;
    }

    if (header->e_ident[0] != ELF_MAGIC0 ||
        header->e_ident[1] != ELF_MAGIC1 ||
        header->e_ident[2] != ELF_MAGIC2 ||
        header->e_ident[3] != ELF_MAGIC3)
    {
        return -1;
    }

    if (header->e_ident[4] != ELFCLASS32)
    {
        return -1;
    }

    if (header->e_ident[5] != ELFDATA2LSB)
    {
        return -1;
    }

    if (header->e_type != ET_EXEC)
    {
        return -1;
    }

    if (header->e_machine != EM_386)
    {
        return -1;
    }

    return 0;
}

int elf_load(const void* data, size_t size, page_directory_t* page_dir, struct elf_load_result* result)
{
    if (!data || !page_dir || !result)
    {
        return -1;
    }

    if (size < sizeof(struct elf32_header))
    {
        return -1;
    }

    const struct elf32_header* header = (const struct elf32_header*)data;

    if (elf_validate(header) != 0)
    {
        return -1;
    }

    if (header->e_phoff == 0 || header->e_phnum == 0)
    {
        return -1;
    }

    if (header->e_phoff + header->e_phnum * sizeof(struct elf32_phdr) > size)
    {
        return -1;
    }

    const struct elf32_phdr* phdrs = (const struct elf32_phdr*)((uint8_t*)data + header->e_phoff);

    result->entry_point = header->e_entry;
    result->brk = 0;

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const struct elf32_phdr* phdr = &phdrs[i];

        if (phdr->p_type != PT_LOAD)
        {
            continue;
        }

        if (phdr->p_memsz == 0)
        {
            continue;
        }

        if (phdr->p_vaddr >= KERNEL_VIRTUAL_BASE)
        {
            return -1;
        }

        uint32_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdr->p_flags & PF_W)
        {
            flags |= PAGE_WRITE;
        }

        uint32_t vaddr = phdr->p_vaddr & ~0xFFF;
        uint32_t vaddr_end = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & ~0xFFF;

        for (uint32_t page = vaddr; page < vaddr_end; page += PAGE_SIZE)
        {
            if (!vmm_is_mapped(page_dir, page))
            {
                if (vmm_alloc_page(page_dir, page, flags) != 0)
                {
                    return -1;
                }
            }
        }

        if (phdr->p_filesz > 0)
        {
            if (phdr->p_offset + phdr->p_filesz > size)
            {
                return -1;
            }

            const uint8_t* src = (const uint8_t*)data + phdr->p_offset;
            uint8_t* dst = (uint8_t*)phdr->p_vaddr;
            memcpy(dst, src, phdr->p_filesz);
        }

        if (phdr->p_memsz > phdr->p_filesz)
        {
            uint8_t* bss_start = (uint8_t*)(phdr->p_vaddr + phdr->p_filesz);
            size_t bss_size = phdr->p_memsz - phdr->p_filesz;
            memset(bss_start, 0, bss_size);
        }

        uint32_t segment_end = phdr->p_vaddr + phdr->p_memsz;
        if (segment_end > result->brk)
        {
            result->brk = segment_end;
        }
    }

    result->brk = (result->brk + 0xFFF) & ~0xFFF;

    return 0;
}

int elf_load_file(const char* path, page_directory_t* page_dir, struct elf_load_result* result)
{
    if (!path || !page_dir || !result)
    {
        return -1;
    }

    static uint8_t file_buffer[FS_MAX_FILE_SIZE];

    int bytes_read = fs_read(path, (char*)file_buffer, FS_MAX_FILE_SIZE);
    if (bytes_read < 0)
    {
        return -1;
    }

    return elf_load(file_buffer, (size_t)bytes_read, page_dir, result);
}
