#include "vmm.h"
#include "pmm.h"
#include "heap.h"
#include "../arch/i686/arch.h"
#include "../core/log.h"
#include "../include/string.h"

extern uint32_t kernel_start;
extern uint32_t kernel_end;

static page_directory_t* kernel_directory = NULL;
static page_directory_t* current_directory = NULL;

static uint32_t kernel_directory_phys = 0;

static void *get_page_table(page_directory_t *page_dir, const uint32_t virt_addr, const bool create)
{
    const uint32_t dir_index = PAGE_DIRECTORY_INDEX(virt_addr);
    uint32_t* dir = (uint32_t*)page_dir;

    if (dir[dir_index] & PAGE_PRESENT)
    {
        const uint32_t table_phys = dir[dir_index] & ~0xFFF;
        // TODO: Proper virtual-to-physical translation
        return (page_table_t*)table_phys;
    }

    if (create)
    {
        page_table_t* table = (page_table_t*)pmm_alloc_block();
        if (!table)
        {
            return NULL;
        }

        memset(table, 0, sizeof(page_table_t));

        uint32_t flags = PAGE_PRESENT | PAGE_WRITE;
        if (virt_addr < KERNEL_VIRTUAL_BASE)
        {
            flags |= PAGE_USER;
        }
        dir[dir_index] = ((uint32_t)table) | flags;

        return table;
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

    memset(page_dir, 0, sizeof(page_directory_t));

    if (kernel_directory)
    {
        const uint32_t* src = (uint32_t*)kernel_directory;
        uint32_t* dst = (uint32_t*)page_dir;
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

    const uint32_t* dir = (uint32_t*)page_dir;

    for (int i = 0; i < 768; i++)
    {
        if (dir[i] & PAGE_PRESENT)
        {
            page_table_t* table = (page_table_t*)(dir[i] & ~0xFFF);
            const uint32_t* table_ptr = (uint32_t*)table;

            for (int j = 0; j < 1024; j++)
            {
                if (table_ptr[j] & PAGE_PRESENT)
                {
                    pmm_free_block((void*)(table_ptr[j] & ~0xFFF));
                }
            }

            pmm_free_block(table);
        }
    }

    pmm_free_block(page_dir);
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

    const uint32_t* src_dir = (uint32_t*)src;
    uint32_t* dst_dir = (uint32_t*)dst;

    for (int i = 0; i < 768; i++)
    {
        if (!(src_dir[i] & PAGE_PRESENT))
        {
            continue;
        }

        page_table_t* src_table = (page_table_t*)(src_dir[i] & ~0xFFF);
        const uint32_t* src_table_ptr = (uint32_t*)src_table;

        page_table_t* dst_table = (page_table_t*)pmm_alloc_block();
        if (!dst_table)
        {
            vmm_destroy_address_space(dst);
            return NULL;
        }

        uint32_t* dst_table_ptr = (uint32_t*)dst_table;
        for (int j = 0; j < 1024; j++)
        {
            if (src_table_ptr[j] & PAGE_PRESENT)
            {
                void* new_phys = pmm_alloc_block();
                if (!new_phys)
                {
                    pmm_free_block(dst_table);
                    vmm_destroy_address_space(dst);
                    return NULL;
                }

                const uint32_t src_phys = src_table_ptr[j] & ~0xFFF;
                memcpy(new_phys, (void*)src_phys, PAGE_SIZE);

                dst_table_ptr[j] = ((uint32_t)new_phys) | (src_table_ptr[j] & 0xFFF);
            }
            else
            {
                dst_table_ptr[j] = 0;
            }
        }

        dst_dir[i] = ((uint32_t)dst_table) | (src_dir[i] & 0xFFF);
    }

    return dst;
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

    uint32_t* dir = (uint32_t*)kernel_directory;
    for (int i = 0; i < 1024; i++)
    {
        dir[i] = 0;
    }

    kernel_directory_phys = (uint32_t)kernel_directory;
    current_directory = kernel_directory;

    log_info("Identity mapping first 8MB");

    // cover 8MB (each tabel covers 4MB = 1024 pages * 4KB)
    for (uint32_t table_idx = 0; table_idx < 2; table_idx++)
    {
        page_table_t* table = (page_table_t*)pmm_alloc_block();
        if (!table)
        {
            log_error("Failed to allocate page table");
            return;
        }

        uint32_t* table_ptr = (uint32_t*)table;
        for (int i = 0; i < 1024; i++)
        {
            table_ptr[i] = 0;
        }

        for (uint32_t i = 0; i < 1024; i++)
        {
            uint32_t phys_addr = (table_idx * 0x400000) + (i * PAGE_SIZE);
            table_ptr[i] = phys_addr | PAGE_PRESENT | PAGE_WRITE;
        }

        dir[table_idx] = ((uint32_t)table) | PAGE_PRESENT | PAGE_WRITE;
    }

    log_info("Enabling paging");
    write_cr3(kernel_directory_phys);

    uint32_t cr0 = read_cr0();
    cr0 |= 0x80000000;
    write_cr0(cr0);

    log_info("Paging enabled - 8MB identity mapped");
}