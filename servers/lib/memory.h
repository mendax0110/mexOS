/**
 * @file memory.h
 * @brief Memory management functions for user-space servers
 *
 * This header provides memory allocation and mapping functions
 * for user-space servers in the mexOS microkernel architecture.
 */

#ifndef SERVER_MEMORY_H
#define SERVER_MEMORY_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory mapping protection flags \enum mem_prot
 */
enum mem_prot
{
    MEM_PROT_READ = 0x01,
    MEM_PROT_WRITE = 0x02,
    MEM_PROT_EXEC = 0x04
};

/**
 * @brief Memory mapping flags \enum mem_flags
 */
enum mem_flags
{
    MEM_FLAG_PRIVATE = 0x01,
    MEM_FLAG_SHARED = 0x02,
    MEM_FLAG_DEVICE = 0x04,
    MEM_FLAG_FIXED = 0x10
};

/**
 * @brief Initialize the server memory allocator
 * @param heap_base Base address of heap memory
 * @param heap_size Size of heap memory in bytes
 * @return 0 on success, negative error code on failure
 */
int mem_init(void *heap_base, uint32_t heap_size);

/**
 * @brief Allocate memory
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void *mem_alloc(uint32_t size);

/**
 * @brief Allocate aligned memory
 * @param size Number of bytes to allocate
 * @param alignment Alignment requirement (must be power of 2)
 * @return Pointer to aligned allocated memory, or NULL on failure
 */
void *mem_alloc_aligned(uint32_t size, uint32_t alignment);

/**
 * @brief Free allocated memory
 * @param ptr Pointer to memory to free
 */
void mem_free(void *ptr);

/**
 * @brief Reallocate memory
 * @param ptr Pointer to existing allocation (may be NULL)
 * @param size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void *mem_realloc(void *ptr, uint32_t size);

/**
 * @brief Allocate zeroed memory
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to zeroed memory, or NULL on failure
 */
void *mem_calloc(uint32_t count, uint32_t size);

/**
 * @brief Map physical memory into server address space
 * @param phys_addr Physical address to map
 * @param size Size of mapping in bytes
 * @param prot Protection flags (mem_prot)
 * @param flags Mapping flags (mem_flags)
 * @return Virtual address of mapping, or NULL on failure
 *
 * This is used to map device memory (MMIO) into the server's
 * address space. Requires appropriate capabilities.
 */
void *mem_map_phys(uint32_t phys_addr, uint32_t size, uint32_t prot, uint32_t flags);

/**
 * @brief Unmap previously mapped memory
 * @param addr Virtual address of mapping
 * @param size Size of mapping
 * @return 0 on success, negative error code on failure
 */
int mem_unmap(void *addr, uint32_t size);

/**
 * @brief Create a shared memory region
 * @param size Size of shared region
 * @param name Optional name for the region (may be NULL)
 * @return Shared memory handle, or negative error code
 */
int mem_share_create(uint32_t size, const char *name);

/**
 * @brief Attach to an existing shared memory region
 * @param handle Shared memory handle
 * @return Pointer to shared memory, or NULL on failure
 */
void *mem_share_attach(int handle);

/**
 * @brief Detach from a shared memory region
 * @param ptr Pointer to shared memory
 * @return 0 on success, negative error code on failure
 */
int mem_share_detach(void *ptr);

/**
 * @brief Get memory statistics
 * @param total_bytes Output: Total heap size
 * @param used_bytes Output: Bytes currently allocated
 * @param free_bytes Output: Bytes available
 */
void mem_stats(uint32_t *total_bytes, uint32_t *used_bytes, uint32_t *free_bytes);

/**
 * @brief Copy memory
 * @param dest Destination buffer
 * @param src Source buffer
 * @param n Number of bytes to copy
 * @return Pointer to destination
 */
void *mem_copy(void *dest, const void *src, uint32_t n);

/**
 * @brief Set memory to a value
 * @param dest Destination buffer
 * @param val Value to set (cast to unsigned char)
 * @param n Number of bytes to set
 * @return Pointer to destination
 */
void *mem_set(void *dest, int val, uint32_t n);

/**
 * @brief Compare memory regions
 * @param s1 First memory region
 * @param s2 Second memory region
 * @param n Number of bytes to compare
 * @return 0 if equal, non-zero otherwise
 */
int mem_cmp(const void *s1, const void *s2, uint32_t n);


#ifdef __cplusplus
}
#endif

#endif // SERVER_MEMORY_H