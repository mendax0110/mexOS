#include "elf.h"
#include "fs/fs.h"
#include "lib/log.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include "include/cast.h"

#define MULTIBOOT_FLAG_ELF_SHDR 0x20

struct multiboot_elf_shdr_info
{
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} PACKED;

struct multiboot_info_min
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    struct multiboot_elf_shdr_info elf_sec;
} PACKED;

static const struct elf32_sym* g_symtab = NULL;
static uint32_t g_symtab_count = 0;
static const char* g_strtab = NULL;
static char g_symbol_buffer[64];

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

int elf_load(const void* data, const size_t size, page_directory_t* page_dir, struct elf_load_result* result)
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

    const struct elf32_header* header = data;

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
            log_warn_fmt("elf_load: segment at virtual address 0x%X is in kernel space", phdr->p_vaddr);
            return -1;
        }

        uint32_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdr->p_flags & PF_W)
        {
            flags |= PAGE_WRITE;
        }

        const uint32_t vaddr = phdr->p_vaddr & ~0xFFF;
        const uint32_t vaddr_end = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & ~0xFFF;

        for (uint32_t page = vaddr; page < vaddr_end; page += PAGE_SIZE)
        {
            if (!vmm_is_mapped(page_dir, page))
            {
                if (vmm_alloc_page(page_dir, page, flags) != 0)
                {
                    log_warn_fmt("elf_load: failed to allocate page for segment at virtual address 0x%X", page);
                    return -1;
                }
            }
        }

        if (phdr->p_filesz > 0)
        {
            const uint8_t* src = (const uint8_t*)data + phdr->p_offset;
            vmm_write_to_page(page_dir, phdr->p_vaddr, src, phdr->p_filesz);
        }

        if (phdr->p_memsz > phdr->p_filesz)
        {
            uint32_t bss_vaddr = phdr->p_vaddr + phdr->p_filesz;
            size_t bss_size = phdr->p_memsz - phdr->p_filesz;
            uint8_t zero = 0;
            while (bss_size > 0)
            {
                const uint32_t page_offset = bss_vaddr & 0xFFF;
                const uint32_t phys = vmm_get_physical_address(page_dir, bss_vaddr);
                uint8_t* dst = phys_to_virt(phys);
                const size_t chunk = (PAGE_SIZE - page_offset < bss_size) ? PAGE_SIZE - page_offset : bss_size;
                memset(dst + page_offset, 0, chunk);
                bss_vaddr += chunk;
                bss_size -= chunk;
            }
        }

        const uint32_t segment_end = phdr->p_vaddr + phdr->p_memsz;
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

static void append_hex_offset(char* buffer, const size_t buf_size, const uint32_t value)
{
    char hex[9];
    for (int i = 7; i >= 0; i--)
    {
        const uint8_t nibble = (value >> (i * 4)) & 0xF;
        hex[7 - i] = nibble < 10 ? (char)('0' + nibble) : (char)('A' + nibble - 10);
    }
    hex[8] = '\0';

    const char* trimmed = hex;
    while (*trimmed == '0' && trimmed[1] != '\0')
    {
        trimmed++;
    }

    const size_t len = strlen(buffer);
    if (len + strlen(trimmed) + 1 < buf_size)
    {
        strcat(buffer, "+0x");
        strcat(buffer, trimmed);
    }
}

void elf_init_symbols(const uint32_t mboot_info)
{
    g_symtab = NULL;
    g_symtab_count = 0;
    g_strtab = NULL;

    if (mboot_info == 0)
    {
        log_warn("elf_init_symbols: no multiboot info provided");
        return;
    }

    const struct multiboot_info_min* mbi = PTR_FROM_U32_TYPED(struct multiboot_info_min, mboot_info);

    if (!(mbi->flags & MULTIBOOT_FLAG_ELF_SHDR))
    {
        log_warn("elf_init_symbols: multiboot info does not contain ELF section headers");
        return;
    }

    const uint32_t shdr_count = mbi->elf_sec.num;
    const uint32_t shdr_size = mbi->elf_sec.size;
    const uint32_t shdr_addr = mbi->elf_sec.addr;
    const uint8_t* shdr_base = PTR_FROM_U32_TYPED(const uint8_t, shdr_addr);

    const struct elf32_shdr* symtab_hdr = NULL;
    const struct elf32_shdr* strtab_hdr = NULL;

    for (uint32_t i = 0; i < shdr_count; i++)
    {
        const struct elf32_shdr* sh = (const struct elf32_shdr*)(shdr_base + i * shdr_size);
        if (sh->sh_type == SHT_SYMTAB)
        {
            symtab_hdr = sh;
        }
    }

    if (!symtab_hdr)
    {
        log_warn("elf_init_symbols: no symbol table section found in ELF headers");
        return;
    }

    if (symtab_hdr->sh_link < shdr_count)
    {
        strtab_hdr = (const struct elf32_shdr*)(shdr_base + symtab_hdr->sh_link * shdr_size);
    }

    if (!strtab_hdr || strtab_hdr->sh_type != SHT_STRTAB)
    {
        log_warn("elf_init_symbols: no valid string table section found for symbol table");
        return;
    }

    g_symtab = PTR_FROM_U32_TYPED(struct elf32_sym, symtab_hdr->sh_addr);
    g_symtab_count = symtab_hdr->sh_size / sizeof(struct elf32_sym);
    g_strtab = PTR_FROM_U32_TYPED(char, strtab_hdr->sh_addr);

    log_info_fmt("elf_init_symbols: loaded %u symbols from ELF symbol table", g_symtab_count);
}

char* elf_find_symtab(void)
{
    return (char*)g_symtab;
}

char* elf_lookup_symbol(const uint32_t addr)
{
    if (!g_symtab || !g_strtab)
    {
        return NULL;
    }

    const struct elf32_sym* best = NULL;

    for (uint32_t i = 0; i < g_symtab_count; i++)
    {
        const struct elf32_sym* sym = &g_symtab[i];

        if (ELF32_ST_TYPE(sym->st_info) != STT_FUNC) continue;
        if (sym->st_value == 0 || sym->st_name == 0) continue;
        if (sym->st_value > addr) continue;
        if (sym->st_size > 0 && addr >= sym->st_value + sym->st_size) continue;

        if (!best || sym->st_value > best->st_value)
        {
            best = sym;
        }
    }

    if (!best)
    {
        return NULL;
    }

    const char* name = g_strtab + best->st_name;
    const uint32_t offset = addr - best->st_value;

    strncpy(g_symbol_buffer, name, sizeof(g_symbol_buffer) - 1);
    g_symbol_buffer[sizeof(g_symbol_buffer) - 1] = '\0';

    if (offset > 0)
    {
        append_hex_offset(g_symbol_buffer, sizeof(g_symbol_buffer), offset);
    }

    return g_symbol_buffer;
}

void elf_reserve_grub_sections(const uint32_t mboot_info)
{
    if (mboot_info == 0) return;

    const struct multiboot_info_min* mbi = PTR_FROM_U32_TYPED(struct multiboot_info_min, mboot_info);

    if (!(mbi->flags & MULTIBOOT_FLAG_ELF_SHDR)) return;

    const uint32_t shdr_count = mbi->elf_sec.num;
    const uint32_t shdr_size = mbi->elf_sec.size;
    const uint32_t shdr_addr = mbi->elf_sec.addr;

    const uint32_t shdr_bytes = shdr_count * shdr_size;
    pmm_deinit_region(shdr_addr, shdr_bytes);

    const uint8_t* base = PTR_FROM_U32_TYPED(const uint8_t, shdr_addr);
    for (uint32_t i = 0; i < shdr_count; i++)
    {
        const struct elf32_shdr* sh = (const struct elf32_shdr*)(base + i * shdr_size);
        if (sh->sh_addr != 0 && sh->sh_size != 0)
        {
            const uint32_t start = sh->sh_addr &~ 0xFFFU;
            const uint32_t end = (sh->sh_addr + sh->sh_size + 0xFFFU) &~ 0xFFFU;
            pmm_deinit_region(start, end - start);
        }
    }
}