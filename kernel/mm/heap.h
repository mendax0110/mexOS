#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the kernel heap
 * @param start The start address of the heap
 * @param size The size of the heap in bytes
 * @return Pointer to the start of the heap
 */
void* heap_init(uint32_t start, uint32_t size);

/**
 * @brief Allocate memory from the kernel heap
 * @param size The size of memory to allocate in bytes
 * @return Pointer to the allocated memory, or NULL on failure
 */
void* kmalloc(size_t size);

/**
 * @brief Allocate aligned memory from the kernel heap
 * @param size The size of memory to allocate in bytes
 * @param align The alignment in bytes
 * @return Pointer to the allocated memory, or NULL on failure
 */
void* kmalloc_aligned(size_t size, size_t align);

/**
 * @brief Free memory allocated from the kernel heap
 * @param ptr Pointer to the memory to free
 */
void kfree(void* ptr);

/**
 * @brief Get the total size of the kernel heap
 * @return The total size of the heap in bytes
 */
size_t heap_get_used(void);

/**
 * @brief Get the free size of the kernel heap
 * @return The free size of the heap in bytes
 */
size_t heap_get_free(void);

#ifdef __cplusplus
}
#endif

#endif
