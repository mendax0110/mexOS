#include "vmm.h"
#include "pmm.h"
#include "alloc_track.h"
#include "arch/i686/arch.h"
#include "lib/log.h"
#include "lib/string.h"
#include "include/addr.h"
#include "core/rollback.h"
#include "ui/console.h"

extern uint32_t kernel_start;
extern uint32_t kernel_end;

static page_directory_t* kernel_directory = NULL;
static page_directory_t* current_directory = NULL;

static uint32_t kernel_directory_phys = 0;

static bool paging_enabled = false;

#define TEMP_MAP_VIRT_BASE 0xFFC00000u
#define TEMP_MAP_VIRT_SIZE 0x00100000u
#define TEMP_MAP_VIRT_END  (TEMP_MAP_VIRT_BASE + TEMP_MAP_VIRT_SIZE)

static uint32_t temp_map_bump = TEMP_MAP_VIRT_BASE;

void* phys_to_virt(const uint32_t phys)
{
    if (kernel_directory_phys == 0)
    {
        log_error_fmt("phys_to_virt called before kernel_directory_phys is set, phys: 0x%x", phys);
        return PTR_FROM_U32(phys);
    }

    // TODO AdrGos: Enabling this check causes a lot of ERR/WARN messages in the log, but it somehow stabilizes the system if we use
    // mexos-gfx.elf/iso (so the userspace with graphics support). But after a couple of minutes the system will still reboot
    // There is no real crash atm (nothing triggers a kernel panic, so we get no stack backtrace and no additional debug information)
    // Maybe it is a triple fault or a page fault that i don't handle correctly/don't catch atm.
    // Might as well read more in here: https://www.brokenthorn.com/Resources/OSDev17.html or here https://www.brokenthorn.com/Resources/OSDev18.html
    if (!paging_enabled)
    {
        // TODO AdrGos: This is an issue, why do i call phys_to_virt before paging is enabled?
        // Might as well check the asm files in boot.s and boot_gfx.s, maybe there is the issue
        log_warn_fmt("phys_to_virt called before paging is enabled, phys: 0x%x", phys);
        return PTR_FROM_U32(phys);
    }

    if (PTR_TO_U32(kernel_directory) < KERNEL_VIRTUAL_BASE)
    {
        // TODO AdrGos: This should actually never happen, but it does later as we can see in the log.
        // first we try to call phys_to_virt too early when paging isn't enabled yet, and then later
        // we try to call it when the kernel dir is not mapped to the virutal address space.
        log_error_fmt("kernel_directory is not mapped to virtual address space, kernel_directory: 0x%x", PTR_TO_U32(kernel_directory));
        return PTR_FROM_U32(phys);
    }

    const uint32_t offset = PTR_TO_U32(kernel_directory) - kernel_directory_phys;
    return PTR_FROM_U32(phys + offset);
}

static void* get_page_table(page_directory_t *page_dir, const uint32_t virt_addr, const bool create)
{
    const uint32_t dir_index = PAGE_DIRECTORY_INDEX(virt_addr);
    uint32_t* dir = phys_to_virt(PTR_TO_U32(page_dir));

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

        const uint32_t table_phys = PTR_TO_U32(table_phys_p);
        page_table_t* table_virt = phys_to_virt(table_phys);
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
        log_error_fmt("Failed to get or create page table for virtual address 0x%x", virt_addr);
        return -1;
    }

    const uint32_t table_index = PAGE_TABLE_INDEX(virt_addr);
    uint32_t* table_ptr = PTR_FROM_U32_TYPED_STRICT(uint32_t, table);
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
        log_error_fmt("Failed to get page table for virtual address 0x%x", virt_addr);
        return;
    }

    const uint32_t table_index = PAGE_TABLE_INDEX(virt_addr);
    uint32_t* table_ptr = PTR_FROM_U32_TYPED_STRICT(uint32_t, table);
    table_ptr[table_index] = 0;

    if (page_dir == current_directory)
    {
        invlpg(virt_addr);
    }
}

uint32_t vmm_get_page_flags(page_directory_t* page_dir, const uint32_t virt_addr)
{
    void* table = get_page_table(page_dir, virt_addr, false);
    if (!table) return 0;
    const uint32_t* entries = PTR_FROM_U32_TYPED_STRICT(uint32_t, table);
    return entries[PAGE_TABLE_INDEX(virt_addr)] & 0xFFFU;
}

uint32_t vmm_get_physical_address(page_directory_t* page_dir, const uint32_t virt_addr)
{
    void *table = get_page_table(page_dir, virt_addr, false);
    if (!table)
    {
        log_error_fmt("Failed to get page table for virtual address 0x%x", virt_addr);
        return 0;
    }

    const uint32_t table_index = PAGE_TABLE_INDEX(virt_addr);
    const uint32_t* table_ptr = PTR_FROM_U32_TYPED_STRICT(uint32_t, table);

    if (!(table_ptr[table_index] & PAGE_PRESENT))
    {
        log_error_fmt("Page not present for virtual address 0x%x", virt_addr);
        return 0;
    }

    return (table_ptr[table_index] & ~0xFFF) | (virt_addr & 0xFFF);
}

bool vmm_is_mapped(page_directory_t* page_dir, const uint32_t virt_addr)
{
    void *table = get_page_table(page_dir, virt_addr, false);
    if (!table)
    {
        return false;
    }

    const uint32_t table_index = PAGE_TABLE_INDEX(virt_addr);
    const uint32_t* table_ptr = PTR_FROM_U32_TYPED_STRICT(uint32_t, table);

    return (table_ptr[table_index] & PAGE_PRESENT) != 0;
}

int vmm_alloc_page(page_directory_t* page_dir, const uint32_t virt_addr, const uint32_t flags)
{
    void* phys = pmm_alloc_block();
    if (!phys) { return -1; }

    TRY_CTX(__FUNCTION__, LAMBDA(void, (void), {
            pmm_free_block(phys);
    }))
    {
        if (vmm_map_page(page_dir, virt_addr, PTR_TO_U32(phys), flags | PAGE_PRESENT) != 0)
        {
            THROW();
        }
    }

    if (phys)
    {
        TRACK_ADD(phys, PAGE_SIZE, ALLOC_SRC_VMM_PAGE);
    }
    return 0;
}

void vmm_free_page(page_directory_t* page_dir, const uint32_t virt_addr)
{
    const uint32_t phys = vmm_get_physical_address(page_dir, virt_addr);
    if (phys)
    {
        pmm_free_block(PTR_FROM_U32(phys & ~0xFFF));
    }
    vmm_unmap_page(page_dir, virt_addr);
    TRACK_REMOVE(PTR_FROM_U32(phys & ~0xFFF), ALLOC_SRC_VMM_PAGE);
}

void* vmm_create_address_space(void)
{
    page_directory_t* page_dir = PTR_FROM_U32_TYPED_STRICT(page_directory_t, pmm_alloc_block());
    if (!page_dir)
    {
        log_error("Failed to allocate page directory for new address space");
        return NULL;
    }

    memset(phys_to_virt(PTR_TO_U32(page_dir)), 0, sizeof(page_directory_t));

    if (kernel_directory)
    {
        const uint32_t* src = (uint32_t*)phys_to_virt(PTR_TO_U32(kernel_directory));
        uint32_t* dst = phys_to_virt(PTR_TO_U32(page_dir));
        for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
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
        log_error_fmt("Attempted to destroy invalid or kernel address space: %p", page_dir);
        return;
    }

    const uint32_t* dir = (uint32_t*)phys_to_virt(PTR_TO_U32(page_dir));

    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        if ((dir[i] & PAGE_PRESENT) && (dir[i] & PAGE_USER))
        {
            const uint32_t table_phys = dir[i] & ~0xFFF;
            const uint32_t* table_ptr = (uint32_t*)phys_to_virt(table_phys);

            for (int j = 0; j < PAGE_DIRECTORY_ENTRIES; j++)
            {
                if ((table_ptr[j] & PAGE_PRESENT) && (table_ptr[j] & PAGE_USER))
                {
                    if (table_ptr[j] & PAGE_CACHE_DISABLE)
                    {
                        continue;
                    }
                    if (table_ptr[j] & PAGE_SHARED)
                    {
                        continue;
                    }

                    pmm_free_block(PTR_FROM_U32(table_ptr[j] & ~0xFFF));
                }
            }

            pmm_free_block(PTR_FROM_U32(table_phys));
        }
    }

    pmm_free_block(PTR_FROM_U32(PTR_TO_U32(page_dir)));
}

void vmm_switch_address_space(page_directory_t* page_dir)
{
    if (!page_dir)
    {
        log_error_fmt("Current directory: %p", current_directory);
        return;
    }

    current_directory = page_dir;

    write_cr3(PTR_TO_U32(page_dir));
}

page_directory_t* vmm_get_current_directory(void)
{
    return current_directory;
}

void* vmm_clone_address_space(page_directory_t* src)
{
    page_directory_t* dst = vmm_create_address_space();
    if (!dst) { return NULL; }

    TRY_CTX(__FUNCTION__, LAMBDA(void, (void), {
            vmm_destroy_address_space(dst);
    }))
    {
        const uint32_t* src_dir = (uint32_t*)phys_to_virt(PTR_TO_U32(src));
        uint32_t* dst_dir = phys_to_virt(PTR_TO_U32(dst));

        for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
        {
            if (!(src_dir[i] & PAGE_PRESENT))
            {
                dst_dir[i] = 0;
                continue;
            }

            if (!(src_dir[i] & PAGE_USER))
            {
                dst_dir[i] = src_dir[i];
                continue;
            }

            const uint32_t src_table_phys = src_dir[i] & ~0xFFF;

            if (src_table_phys == 0)
            {
                dst_dir[i] = src_dir[i];
                continue;
            }

            const uint32_t* src_table_ptr = (uint32_t*)phys_to_virt(src_table_phys);

            void* dst_table_phys_p = pmm_alloc_block();
            if (!dst_table_phys_p) THROW();

            const uint32_t dst_table_phys = PTR_TO_U32(dst_table_phys_p);
            uint32_t* dst_table_ptr = phys_to_virt(dst_table_phys);

            bool cloned_fame[PAGE_DIRECTORY_ENTRIES] = { false };

            for (int j = 0; j < PAGE_DIRECTORY_ENTRIES; j++)
            {
                if (!(src_table_ptr[j] & PAGE_PRESENT))
                {
                    dst_table_ptr[j] = 0;
                    continue;
                }

                const uint32_t src_phys = src_table_ptr[j] & ~0xFFF;

                if (!(src_table_ptr[j] & PAGE_USER) || src_phys == 0)
                {
                    dst_table_ptr[j] = src_table_ptr[j];
                    continue;
                }

                if (src_table_ptr[j] & PAGE_SHARED)
                {
                    dst_table_ptr[j] = src_table_ptr[j];
                    continue;
                }

                void* src_virt = phys_to_virt(src_phys);
                if (!src_virt)
                {
                    dst_table_ptr[j] = src_table_ptr[j];
                    continue;
                }

                void* new_phys_p = pmm_alloc_block();
                if (!new_phys_p)
                {
                    for (int k = 0; k < j; k++)
                    {
                        if (cloned_fame[k])
                        {
                            pmm_free_block(PTR_FROM_U32(dst_table_ptr[k] & ~0xFFF));
                        }
                    }
                    pmm_free_block(PTR_FROM_U32(dst_table_phys));
                    THROW();
                }

                const uint32_t new_phys = PTR_TO_U32(new_phys_p);
                void* dst_virt = phys_to_virt(new_phys);

                memcpy(dst_virt, src_virt, PAGE_SIZE);
                dst_table_ptr[j] = new_phys | (src_table_ptr[j] & 0xFFF);
                cloned_fame[j] = true;
            }

            dst_dir[i] = dst_table_phys | (src_dir[i] & 0xFFF);
        }
    }

    return dst;
}

bool vmm_check_user_ptr(const void* ptr, const size_t len, const bool write)
{
    if (!ptr) { return false; }
    if (len == 0) { return true; }

    const uint32_t start = PTR_TO_U32(ptr);

    //Guard against start+len wrapping around to 0
    if (len > USER_SPACE_END + 1U) { return false; }
    if (start > USER_SPACE_END) { return false; }

    const uint32_t end = start + (uint32_t)len - 1U;
    if (end < start) { return false; }
    if (end > USER_SPACE_END) { return false; }

    page_directory_t* pd = vmm_get_current_directory();
    if (!pd) { return false; }

    uint32_t page = start & ~0xFFF;
    while (page <= end)
    {
        const uint32_t dir_index = PAGE_DIRECTORY_INDEX(page);
        const uint32_t* dir = (uint32_t*)phys_to_virt(PTR_TO_U32(pd));
        if (!(dir[dir_index] & PAGE_PRESENT)) { return false; }

        const uint32_t table_phys = dir[dir_index] & ~0xFFF;
        const uint32_t* table = (uint32_t*)phys_to_virt(table_phys);
        const uint32_t table_index = PAGE_TABLE_INDEX(page);
        const uint32_t entry = table[table_index];
        if (!(entry & PAGE_PRESENT)) { return false; }
        if (!(entry & PAGE_USER)) { return false; }
        if (write && !(entry & PAGE_WRITE)) { return false; }

        page += PAGE_SIZE;
    }

    return true;
}

void vmm_init(void)
{
    log_info("Initializing Virtual Memory Manager");

    kernel_directory = PTR_FROM_U32_TYPED_STRICT(page_directory_t, pmm_alloc_block());
    if (!kernel_directory)
    {
        log_error("Failed to allocate kernel page directory");
        return;
    }

    kernel_directory_phys = PTR_TO_U32(kernel_directory);
    current_directory = kernel_directory;

    uint32_t* dir = phys_to_virt(PTR_TO_U32(kernel_directory));
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
    {
        dir[i] = 0;
    }

    log_info("Identity mapping first 128MB");

    // covers 8MB (each table covers 4MB = 1024 pages * 4KB)
    for (uint32_t table_idx = 0; table_idx < 32; table_idx++)
    {
        void* table_phys_p = pmm_alloc_block();
        if (!table_phys_p)
        {
            log_error("Failed to allocate page table");
            return;
        }

        const uint32_t table_phys = PTR_TO_U32(table_phys_p);
        uint32_t* table_ptr = phys_to_virt(table_phys);

        for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++)
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
    paging_enabled = true;

    log_info("Paging enabled - 128MB identity mapped");
}

page_directory_t* vmm_get_kernel_directory(void)
{
    return kernel_directory;
}

void vmm_write_to_page(page_directory_t* page_dir, uint32_t virt_addr, const void* data, size_t len)
{
    const uint8_t* src = data;
    while (len > 0)
    {
        const uint32_t page_offset = virt_addr & 0xFFF;
        const uint32_t phys = vmm_get_physical_address(page_dir, virt_addr);
        uint8_t* dst = phys_to_virt(phys);

        const size_t chunk = (PAGE_SIZE - page_offset < len) ? PAGE_SIZE - page_offset : len;

        memcpy(dst, src, chunk);

        virt_addr += chunk;
        src += chunk;
        len -= chunk;
    }
}

void vmm_shutdown(void)
{
    log_info("Shutting down Virtual Memory Manager");

    if (current_directory != kernel_directory)
    {
        vmm_switch_address_space(kernel_directory);
    }

    vmm_destroy_address_space(current_directory);
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: vmm driver shutdown complete\n", __FUNCTION__);
    console_write(msg);
}

void* vmm_map_temp(const uint32_t phys_addr, const uint32_t size)
{
    if (size == 0)
    {
        log_error("vmm_map_temp called with size 0");
        return NULL;
    }

    const uint32_t phys_page_base = phys_addr & ~0xFFF;
    const uint32_t offset_in_page = phys_addr & 0xFFF;
    const uint32_t span = offset_in_page + size;
    const uint32_t page_count = (span + PAGE_SIZE - 1) / PAGE_SIZE;
    const uint32_t bytes_needed = page_count * PAGE_SIZE;

    if (temp_map_bump + bytes_needed > TEMP_MAP_VIRT_END)
    {
        log_error_fmt("vmm_map_temp: Not enough temporary virtual address space to map 0x%x bytes", size);
        return NULL;
    }

    const uint32_t virt_base = temp_map_bump;

    for (uint32_t i = 0; i < page_count; i++)
    {
        const uint32_t virt = virt_base + (i * PAGE_SIZE);
        const uint32_t phys = phys_page_base + (i * PAGE_SIZE);

        if (vmm_map_page(kernel_directory, virt, phys, PAGE_PRESENT | PAGE_WRITE) != 0)
        {
            log_error_fmt("vmm_map_temp: Failed to map page 0x%x to 0x%x", phys, virt);

            for (uint32_t j = 0; j < i; j++)
            {
                vmm_unmap_page(kernel_directory, virt_base + (j * PAGE_SIZE));
            }

            return NULL;
        }
    }

    temp_map_bump += bytes_needed;

    return PTR_FROM_U32(virt_base + offset_in_page);
}

void vmm_unmap_temp(void* virt_addr, const uint32_t size)
{
    if (!virt_addr || size == 0)
    {
        log_error("vmm_unmap_temp called with invalid parameters");
        return;
    }

    const uint32_t virt = PTR_TO_U32(virt_addr);
    const uint32_t virt_page_base = virt & ~0xFFF;
    const uint32_t offset_in_page = virt & 0xFFF;
    const uint32_t page_count = (offset_in_page + size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i < page_count; i++)
    {
        vmm_unmap_page(kernel_directory, virt_page_base + (i * PAGE_SIZE));
    }
}
