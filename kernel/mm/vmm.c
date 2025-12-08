#include "vmm.h"
#include "pmm.h"
#include "heap.h"
#include "../arch/i686/arch.h"
#include "../lib/log.h"
#include "../include/string.h"

extern uint32_t kernel_start;
extern uint32_t kernel_end;

static page_directory_t* kernel_directory = NULL;
static page_directory_t* current_directory = NULL;

static uint32_t kernel_directory_phys = 0;

static inline void* phys_to_virt(uint32_t phys)
{
    if (kernel_directory_phys == 0)
    {
        return (void*)phys;
    }

    if ((uint32_t)kernel_directory < KERNEL_VIRTUAL_BASE)
    {
        return (void*)phys;
    }

    const uint32_t offset = (uint32_t)kernel_directory - kernel_directory_phys;
    return (void*)(phys + offset);
}

static void *get_page_table(page_directory_t *page_dir, const uint32_t virt_addr, const bool create)
{
    const uint32_t dir_index = PAGE_DIRECTORY_INDEX(virt_addr);
    uint32_t* dir = (uint32_t*)phys_to_virt((uint32_t)page_dir);

    if (dir[dir_index] & PAGE_PRESENT)
    {
        const uint32_t table_phys = dir[dir_index] & ~0xFFF;
        return (page_table_t*)phys_to_virt(table_phys);
    }

    if (create)
    {
        void* table_phys_p = pmm_alloc_block();
        if (!table_phys_p)
        {
            return NULL;
        }

        const uint32_t table_phys = (uint32_t)table_phys_p;
        page_table_t* table_virt = (page_table_t*)phys_to_virt(table_phys);
        memset(table_virt, 0, sizeof(page_table_t));

        uint32_t flags = PAGE_PRESENT | PAGE_WRITE;
        if (virt_addr < KERNEL_VIRTUAL_BASE)
        {
            flags |= PAGE_USER;
        }
        dir[dir_index] = table_phys | flags;

        return table_virt;
    }

    return NULL;
}

int vmm_map_page(page_directory_t* page_dir, uint32_t virt_addr, uint32_t phys_addr, const uint32_t flags)
{
    virt_addr &= ~0xFFF;
    phys_addr &= ~0xFFF;

    void *table = get_page_table(page_dir, virt_addr, true);
    if (!table)
    {
        return -1;
    }

    const uint32_t table_index = PAGE_TABLE_INDEX(virt_addr);
    uint32_t* table_ptr = (uint32_t*)table;
    table_ptr[table_index] = phys_addr | flags;

    if (page_dir == current_directory)
    {
        invlpg(virt_addr);
    }

    return 0;
}

void vmm_unmap_page(page_directory_t* page_dir, uint32_t virt_addr)
{
    virt_addr &= ~0xFFF;

    void *table = get_page_table(page_dir, virt_addr, false);
    if (!table)
    {
        return;
    }

    const uint32_t table_index = PAGE_TABLE_INDEX(virt_addr);
    uint32_t* table_ptr = (uint32_t*)table;
    table_ptr[table_index] = 0;

    if (page_dir == current_directory)
    {
        invlpg(virt_addr);
    }
}

uint32_t vmm_get_physical_address(page_directory_t* page_dir, const uint32_t virt_addr)
{
    void *table = get_page_table(page_dir, virt_addr, false);
    if (!table)
    {
        return 0;
    }

    const uint32_t table_index = PAGE_TABLE_INDEX(virt_addr);
    uint32_t* table_ptr = (uint32_t*)table;

    if (!(table_ptr[table_index] & PAGE_PRESENT))
    {
        return 0;
    }

    return (table_ptr[table_index] & ~0xFFF) | (virt_addr & 0xFFF);
}

bool vmm_is_mapped(page_directory_t* page_dir, uint32_t virt_addr)
{
    void *table = get_page_table(page_dir, virt_addr, false);
    if (!table)
    {
        return false;
    }

    const uint32_t table_index = PAGE_TABLE_INDEX(virt_addr);
    uint32_t* table_ptr = (uint32_t*)table;

    return (table_ptr[table_index] & PAGE_PRESENT) != 0;
}

int vmm_alloc_page(page_directory_t* page_dir, const uint32_t virt_addr, const uint32_t flags)
{
    void* phys = pmm_alloc_block();
    if (!phys)
    {
        return -1;
    }

    if (vmm_map_page(page_dir, virt_addr, (uint32_t)phys, flags | PAGE_PRESENT) != 0)
    {
        pmm_free_block(phys);
        return -1;
    }

    return 0;
}

void vmm_free_page(page_directory_t* page_dir, const uint32_t virt_addr)
{
    const uint32_t phys = vmm_get_physical_address(page_dir, virt_addr);
    if (phys)
    {
        pmm_free_block((void*)(phys & ~0xFFF));
    }
    vmm_unmap_page(page_dir, virt_addr);
}

void *vmm_create_address_space(void)
{
    page_directory_t* page_dir = (page_directory_t*)pmm_alloc_block();
    if (!page_dir)
    {
        return NULL;
    }

    memset(phys_to_virt((uint32_t)page_dir), 0, sizeof(page_directory_t));

    if (kernel_directory)
    {
        const uint32_t* src = (uint32_t*)phys_to_virt((uint32_t)kernel_directory);
        uint32_t* dst = (uint32_t*)phys_to_virt((uint32_t)page_dir);
        for (int i = 768; i < 1024; i++)
        {
            dst[i] = src[i];
        }
    }

    return page_dir;
}

void vmm_destroy_address_space(page_directory_t* page_dir)
{
    if (!page_dir || page_dir == kernel_directory)
    {
        return;
    }

    const uint32_t* dir = (uint32_t*)phys_to_virt((uint32_t)page_dir);

    for (int i = 0; i < 768; i++)
    {
        if (dir[i] & PAGE_PRESENT)
        {
            const uint32_t table_phys = dir[i] & ~0xFFF;
            uint32_t* table_ptr = (uint32_t*)phys_to_virt(table_phys);

            for (int j = 0; j < 1024; j++)
            {
                if (table_ptr[j] & PAGE_PRESENT)
                {
                    pmm_free_block((void*)(table_ptr[j] & ~0xFFF));
                }
            }

            pmm_free_block((void*)table_phys);
        }
    }

    pmm_free_block((void*)((uint32_t)page_dir));
}

void vmm_switch_address_space(page_directory_t* page_dir)
{
    if (!page_dir)
    {
        return;
    }

    current_directory = page_dir;
    write_cr3((uint32_t)page_dir);
}

page_directory_t* vmm_get_current_directory(void)
{
    return current_directory;
}

void* vmm_clone_address_space(page_directory_t *src)
{
    page_directory_t *dst = vmm_create_address_space();
    if (!dst)
    {
        return NULL;
    }

    const uint32_t* src_dir = (uint32_t*)phys_to_virt((uint32_t)src);
    uint32_t* dst_dir = (uint32_t*)phys_to_virt((uint32_t)dst);

    for (int i = 0; i < 768; i++)
    {
        if (!(src_dir[i] & PAGE_PRESENT))
        {
            continue;
        }

        const uint32_t src_table_phys = src_dir[i] & ~0xFFF;
        const uint32_t* src_table_ptr = (uint32_t*)phys_to_virt(src_table_phys);

        void* dst_table_phys_p = pmm_alloc_block();
        if (!dst_table_phys_p)
        {
            vmm_destroy_address_space(dst);
            return NULL;
        }

        const uint32_t dst_table_phys = (uint32_t)dst_table_phys_p;
        uint32_t* dst_table_ptr = (uint32_t*)phys_to_virt(dst_table_phys);
        for (int j = 0; j < 1024; j++)
        {
            if (src_table_ptr[j] & PAGE_PRESENT)
            {
                void* new_phys_p = pmm_alloc_block();
                if (!new_phys_p)
                {
                    pmm_free_block((void*)dst_table_phys);
                    vmm_destroy_address_space(dst);
                    return NULL;
                }

                const uint32_t new_phys = (uint32_t)new_phys_p;
                const uint32_t src_phys = src_table_ptr[j] & ~0xFFF;
                memcpy(phys_to_virt(new_phys), phys_to_virt(src_phys), PAGE_SIZE);

                dst_table_ptr[j] = new_phys | (src_table_ptr[j] & 0xFFF);
            }
            else
            {
                dst_table_ptr[j] = 0;
            }
        }

        dst_dir[i] = dst_table_phys | (src_dir[i] & 0xFFF);
    }

    return dst;
}

bool vmm_check_user_ptr(const void* ptr, size_t len, const bool write)
{
    if (!ptr) return false;
    if (len == 0) return true;

    const uint32_t start = (uint32_t)ptr;
    const uint32_t end = start + len - 1;

    if (start > USER_SPACE_END || end > USER_SPACE_END)
    {
        return false;
    }

    page_directory_t* pd = vmm_get_current_directory();
    if (!pd) return false;

    uint32_t page = start & ~0xFFF;
    while (page <= end)
    {
        const uint32_t dir_index = PAGE_DIRECTORY_INDEX(page);
        const uint32_t* dir = (uint32_t*)phys_to_virt((uint32_t)pd);
        if (!(dir[dir_index] & PAGE_PRESENT)) return false;

        const uint32_t table_phys = dir[dir_index] & ~0xFFF;
        const uint32_t* table = (uint32_t*)phys_to_virt(table_phys);
        const uint32_t table_index = PAGE_TABLE_INDEX(page);
        const uint32_t entry = table[table_index];
        if (!(entry & PAGE_PRESENT)) return false;
        if (!(entry & PAGE_USER)) return false;
        if (write && !(entry & PAGE_WRITE)) return false;

        page += PAGE_SIZE;
    }

    return true;
}

void vmm_init(void)
{
    log_info("Initializing Virtual Memory Manager");

    kernel_directory = (page_directory_t*)pmm_alloc_block();
    if (!kernel_directory)
    {
        log_error("Failed to allocate kernel page directory");
        return;
    }

    kernel_directory_phys = (uint32_t)kernel_directory;
    current_directory = kernel_directory;

    uint32_t* dir = (uint32_t*)phys_to_virt((uint32_t)kernel_directory);
    for (int i = 0; i < 1024; i++)
    {
        dir[i] = 0;
    }

    log_info("Identity mapping first 8MB");

    // cover 8MB (each tabel covers 4MB = 1024 pages * 4KB)
    for (uint32_t table_idx = 0; table_idx < 2; table_idx++)
    {
        void* table_phys_p = pmm_alloc_block();
        if (!table_phys_p)
        {
            log_error("Failed to allocate page table");
            return;
        }

        const uint32_t table_phys = (uint32_t)table_phys_p;
        uint32_t* table_ptr = (uint32_t*)phys_to_virt(table_phys);
        for (int i = 0; i < 1024; i++)
        {
            table_ptr[i] = 0;
        }

        for (uint32_t i = 0; i < 1024; i++)
        {
            const uint32_t phys_addr = (table_idx * 0x400000) + (i * PAGE_SIZE);
            table_ptr[i] = phys_addr | PAGE_PRESENT | PAGE_WRITE;
        }

        dir[table_idx] = table_phys | PAGE_PRESENT | PAGE_WRITE;
    }

    log_info("Enabling paging");
    write_cr3(kernel_directory_phys);

    uint32_t cr0 = read_cr0();
    cr0 |= 0x80000000;
    write_cr0(cr0);

    log_info("Paging enabled - 8MB identity mapped");
}