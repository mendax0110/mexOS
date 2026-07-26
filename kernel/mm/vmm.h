#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include "../../shared/types.h"
#include "mm/page.h"

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
 * @brief Get the flags of a virtual page
 * @param page_dir The page directory to look up in
 * @param virt_addr Virtual address
 * @return Page flags (PAGE_PRESENT, PAGE_WRITE, PAGE_USER, etc.), or 0 if not mapped
 */
uint32_t vmm_get_page_flags(page_directory_t* page_dir, uint32_t virt_addr);

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
void* vmm_clone_address_space(page_directory_t *src);

 /**
 * @brief Validate a user pointer range is mapped and accessible
 * @param ptr User pointer
 * @param len Length in bytes
 * @param write True if the caller intends to write to the buffer
 * @return true if the buffer is valid, false otherwise
 */
bool vmm_check_user_ptr(const void* ptr, size_t len, bool write);

/**
 * @brief Convert a physical address to a virtual address in the kernel space
 * @param phys Physical address
 * @return Virtual address corresponding to the physical address
 */
void* phys_to_virt(uint32_t phys);

/**
 * @brief Get the kernel's page directory
 * @return A pointer to the kernel's page directory
 */
page_directory_t* vmm_get_kernel_directory(void);

/**
 * @brief Write data to a virtual address in a page
 * @param page_dir The page directory to write in
 * @param virt_addr Virtual address to write to
 * @param data Pointer to the data to write
 * @param len Length of data in bytes
 */
void vmm_write_to_page(page_directory_t* page_dir, uint32_t virt_addr, const void* data, size_t len);

/**
 * @brief Shutdown the Virtual Memory Manager and free all resources
 */
void vmm_shutdown(void);

/**
 * @brief Map a physical address to a temporary virtual address for kernel access
 * @param phys_addr Physical address to map
 * @param size Size of the mapping in bytes
 * @return Virtual address corresponding to the mapped physical address
 */
void* vmm_map_temp(uint32_t phys_addr, uint32_t size);

/**
 * @brief Unmap a temporary virtual address mapping
 * @param virt_addr Virtual address to unmap
 * @param size Size of the mapping in bytes
 */
void vmm_unmap_temp(void* virt_addr, uint32_t size);

#endif
