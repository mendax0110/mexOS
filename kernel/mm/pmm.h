#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include "../include/types.h"
#include "../include/config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Physical Memory Manager (PMM)
 * @param mem_size Total memory size in bytes
 * @param bitmap_addr The address to store the PMM bitmap
 */
void pmm_init(uint32_t mem_size, uint32_t bitmap_addr);

/**
 * @brief Initialize a memory region for PMM
 * @param base The base address of the region
 * @param size The size of the region in bytes
 */
void pmm_init_region(uint32_t base, uint32_t size);

/**
 * @brief Deinitialize a memory region from PMM
 * @param base The base address of the region
 * @param size The size of the region in bytes
 */
void pmm_deinit_region(uint32_t base, uint32_t size);

/**
 * @brief Allocate a single memory block
 * @return Pointer to the allocated block, or NULL on failure
 */
void* pmm_alloc_block(void);

/**
 * @brief Free a single memory block
 * @param p Pointer to the block to free
 */
void pmm_free_block(void* p);

/**
 * @brief Allocate multiple contiguous memory blocks
 * @param count Number of blocks to allocate
 * @return Pointer to the allocated blocks, or NULL on failure
 */
void* pmm_alloc_blocks(uint32_t count);

/**
 * @brief Free multiple contiguous memory blocks
 * @param p Pointer to the blocks to free
 * @param count Number of blocks to free
 */
void pmm_free_blocks(void* p, uint32_t count);

/**
 * @brief Get the total memory size managed by PMM
 * @return Total memory size in bytes
 */
uint32_t pmm_get_memory_size(void);

/**
 * @brief Get the total number of memory blocks managed by PMM
 * @return Total number of blocks
 */
uint32_t pmm_get_block_count(void);

/**
 * @brief Get the number of used memory blocks
 * @return Number of used blocks
 */
uint32_t pmm_get_used_block_count(void);

/**
 * @brief Get the number of free memory blocks
 * @return Number of free blocks
 */
uint32_t pmm_get_free_block_count(void);

#ifdef __cplusplus
}
#endif

#endif
