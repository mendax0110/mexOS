#ifndef KERNEL_ALLOC_TRACK_H
#define KERNEL_ALLOC_TRACK_H

#include "../../shared/types.h"
#include "include/source_location.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ALLOC_TRACK_MAX 4096

/**
 * @brief Allocation source types \enum alloc_src_t
 */
typedef enum
{
    ALLOC_SRC_KMALLOC,
    ALLOC_SRC_PMM_BLOCK,
    ALLOC_SRC_PMM_BLOCKS,
    ALLOC_SRC_VMM_PAGE,
    ALLOC_SRC_THREAD_CONTEXT
} alloc_src_t;

/**
 * @brief Allocation tracker add
 * @param ptr Pointer to the allocated memory
 * @param size Size of the allocated memory
 * @param src Source of the allocation
 * @param file File name where the allocation occurred
 * @param line Line number where the allocation occurred
 */
void alloc_track_add(void* ptr, size_t size, alloc_src_t src, const char* file, int line);

/**
 * @brief Allocation tracker remove
 * @param ptr Pointer to the allocated memory
 * @param src Source of the allocation
 */
void alloc_track_remove(void* ptr, alloc_src_t src, const char* file, int line);

/**
 * @brief Dump the allocation tracker information
 */
void alloc_track_dump(void);

/**
 * @brief Get the count of live allocations
 * @return Count of live allocations
 */
uint32_t alloc_track_live_count(void);

/**
 * @brief Get the total size of live allocations
 * @return Total size of live allocations in bytes
 */
uint32_t alloc_track_live_bytes(void);

/**
 * @brief Check if the allocation tracker is initialized
 * @return true if initialized, false otherwise
 */
bool alloc_track_is_initialized(void);

/**
 * @brief Initialize the allocation tracker
 */
void alloc_track_init(void);

/**
 * @brief Macro to add an allocation to the tracker
 */
#define TRACK_ADD(ptr, size, src) \
    alloc_track_add((ptr), (size), (src), __FILENAME__, __LINE__)

/**
 * @brief Macro to remove an allocation from the tracker
 */
#define TRACK_REMOVE(ptr, src) \
    alloc_track_remove((ptr), (src), __FILENAME__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif // KERNEL_ALLOC_TRACK_H
