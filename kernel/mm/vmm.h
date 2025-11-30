#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Page size (4KB)
 */
#define PAGE_SIZE 0x1000

/**
 * @brief Kernel virtual base address (higher half at 3GB)
 */
#define KERNEL_VIRTUAL_BASE 0xC0000000

/**
 * @brief Maximum user space address
 */
#define USER_SPACE_END 0xBFFFFFFF

/**
 * @brief Page directory and table indices
 */
#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0xFFF)

/**
 * @brief Page flags
 */
#define PAGE_PRESENT    0x001  // Page is present in memory
#define PAGE_WRITE      0x002  // Page is writable
#define PAGE_USER       0x004  // Page is accessible from user mode
#define PAGE_WRITETHROUGH 0x008  // Write-through caching
#define PAGE_CACHE_DISABLE 0x010  // Cache disabled
#define PAGE_ACCESSED   0x020  // Page was accessed
#define PAGE_DIRTY      0x040  // Page was written to
#define PAGE_SIZE_BIT   0x080  // 4MB page (if enabled)
#define PAGE_GLOBAL     0x100  // Global page (not flushed from TLB)

/**
 * @brief Page directory entry type (1024 entries)
 */
typedef uint32_t page_directory_t[1024] ALIGNED(4096);

/**
 * @brief Page table entry type (1024 entries)
 */
typedef uint32_t page_table_t[1024] ALIGNED(4096);

/**
 * @brief Initialize the Virtual Memory Manager
 * Enables paging and sets up the kernel's page directory
 */
void vmm_init(void);

/**
 * @brief Create a new page directory for a process
 * @return Pointer to the new page directory, or NULL on failure
 */
void *vmm_create_address_space(void);

/**
 * @brief Destroy a page directory and free all associated page tables
 * @param page_dir The page directory to destroy
 */
void vmm_destroy_address_space(page_directory_t* page_dir);

/**
 * @brief Switch to a different address space
 * @param page_dir The page directory to switch to
 */
void vmm_switch_address_space(page_directory_t* page_dir);

/**
 * @brief Get the current page directory
 * @return Pointer to the current page directory
 */
page_directory_t* vmm_get_current_directory(void);

/**
 * @brief Map a virtual page to a physical page
 * @param page_dir The page directory to map in
 * @param virt_addr Virtual address (page-aligned)
 * @param phys_addr Physical address (page-aligned)
 * @param flags Page flags (PAGE_PRESENT, PAGE_WRITE, PAGE_USER, etc.)
 * @return 0 on success, -1 on failure
 */
int vmm_map_page(page_directory_t* page_dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

/**
 * @brief Unmap a virtual page
 * @param page_dir The page directory to unmap from
 * @param virt_addr Virtual address (page-aligned)
 */
void vmm_unmap_page(page_directory_t* page_dir, uint32_t virt_addr);

/**
 * @brief Get the physical address mapped to a virtual address
 * @param page_dir The page directory to look up in
 * @param virt_addr Virtual address
 * @return Physical address, or 0 if not mapped
 */
uint32_t vmm_get_physical_address(page_directory_t* page_dir, uint32_t virt_addr);

/**
 * @brief Check if a virtual address is mapped
 * @param page_dir The page directory to check
 * @param virt_addr Virtual address
 * @return true if mapped, false otherwise
 */
bool vmm_is_mapped(page_directory_t* page_dir, uint32_t virt_addr);

/**
 * @brief Allocate and map a page for a virtual address
 * @param page_dir The page directory to map in
 * @param virt_addr Virtual address (page-aligned)
 * @param flags Page flags
 * @return 0 on success, -1 on failure
 */
int vmm_alloc_page(page_directory_t* page_dir, uint32_t virt_addr, uint32_t flags);

/**
 * @brief Unmap and free a page
 * @param page_dir The page directory to unmap from
 * @param virt_addr Virtual address (page-aligned)
 */
void vmm_free_page(page_directory_t* page_dir, uint32_t virt_addr);

/**
 * @brief Clone a page directory (for fork())
 * @param src The source page directory to clone
 * @return Pointer to the cloned page directory, or NULL on failure
 */
void *vmm_clone_address_space(page_directory_t *src);

#ifdef __cplusplus
}
#endif

#endif
