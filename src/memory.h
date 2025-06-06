#pragma once

#include "dataTypes.h"

/// @brief Global variables for user stack and kernel heap.
extern uint8_t user_stack[4096];
extern uint8_t kernel_heap[4096 * 1024];

/// @brief Size definitions for memory segments in the kernel.
#define KERNEL_STACK_SIZE 4096
#define USER_STACK_SIZE   sizeof(user_stack)
#define KERNEL_HEAP_SIZE  sizeof(kernel_heap)

/// @brief Class representing a memory pool for dynamic memory allocation.
struct BlockHeader
{
    uint32_t size;
    bool used;
    BlockHeader* next;
};

/// @brief Class representing a memory pool for dynamic memory allocation. \class MemoryPool
class MemoryPool
{
public:

    /**
     * @brief Gets the singleton instance of the MemoryPool.
     * @return A reference to the MemoryPool instance.
     */
    static MemoryPool& instance();

    /**
     * @brief Initializes the memory pool.
     */
    void initialize();

    /**
     * @brief Allocates a block of memory from the pool.
     * @param size The size of the memory block to allocate.
     * @return A pointer to the allocated memory block, or nullptr if allocation fails.
     */
    void* allocate(uint32_t size);

    /**
     * @brief Frees a previously allocated block of memory.
     * @param ptr The pointer to the memory block to free.
     */
    void free(void* ptr);

    /**
     * @brief Gets the amount of used memory in the pool.
     * @return The amount of used memory in bytes.
     */
    uint32_t getUsedMemory() const;

    /**
     * @brief Gers the amount of free memory in the pool.
     * @return The amount of free memory in bytes.
     */
    uint32_t getFreeMemory() const;

    /**
     * @brief Resets the memory pool, freeing all allocated memory.
     */
    void reset();

    /**
     * @brief Prints the memory statistics of the pool.
     */
    void printMemoryStats() const;

private:
    uint8_t* pool_start = nullptr;
    uint32_t pool_size = 0;
    BlockHeader* free_list = nullptr;
    uint32_t used_memory = 0;

    /**
     * @brief Aligns the size to a multiple of 4 bytes.
     * @param size The size to align.
     * @return The aligned size.
     */
    static constexpr uint32_t align(uint32_t size)
    {
        return (size + 3) & ~3;
    }

    MemoryPool() = default;
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
};