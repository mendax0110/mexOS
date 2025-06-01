#include "memory.h"
#include "mexKernel.h"
#include "kernelUtils.h"

uint8_t user_stack[4096];
uint8_t kernel_heap[4096 * 1024]; // 4MB heap for the kernel

MemoryPool& MemoryPool::instance()
{
    static MemoryPool instance;
    return instance;
}

void MemoryPool::initialize()
{
    pool_start = kernel_heap;
    pool_size = sizeof(kernel_heap);
    used_memory = 0;

    free_list = (BlockHeader*)pool_start;
    free_list->size = pool_size - sizeof(BlockHeader);
    free_list->used = false;
    free_list->next = nullptr;
}

void* MemoryPool::allocate(uint32_t size)
{
    size = align(size);
    BlockHeader* current = free_list;
    BlockHeader* prev = nullptr;

    while (current)
    {
        if (!current->used && current->size >= size)
        {
            // Split block if there's space for another header + data
            if (current->size >= size + sizeof(BlockHeader) + 4)
            {
                BlockHeader* new_block = (BlockHeader*)((uint8_t*)current + sizeof(BlockHeader) + size);
                new_block->size = current->size - size - sizeof(BlockHeader);
                new_block->used = false;
                new_block->next = current->next;
                current->next = new_block;
                current->size = size;
            }

            current->used = true;
            used_memory += size + sizeof(BlockHeader);
            return (uint8_t*)current + sizeof(BlockHeader);
        }

        prev = current;
        current = current->next;
    }

    return nullptr; // No suitable block
}

void MemoryPool::free(void* ptr)
{
    if (!ptr) return;

    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    block->used = false;
    used_memory -= block->size + sizeof(BlockHeader);

    BlockHeader* current = free_list;
    while (current)
    {
        if (!current->used)
        {
            BlockHeader* next = current->next;
            while (next && !next->used &&
                   (uint8_t*)current + sizeof(BlockHeader) + current->size == (uint8_t*)next)
            {
                current->size += sizeof(BlockHeader) + next->size;
                current->next = next->next;
                next = next->next;
            }
        }
        current = current->next;
    }
}

uint32_t MemoryPool::getUsedMemory() const
{
    return used_memory;
}

uint32_t MemoryPool::getFreeMemory() const
{
    return pool_size - used_memory;
}

void MemoryPool::reset()
{
    initialize();
}

void MemoryPool::printMemoryStats() const
{
    Kernel::instance()->terminal().write("Memory Pool Blocks:\n");

    BlockHeader* current = (BlockHeader*)pool_start;
    while (current)
    {
        Kernel::instance()->terminal().write("  - Block at ");
        Kernel::instance()->terminal().write(hex_to_str((uint32_t)current));
        Kernel::instance()->terminal().write(": size=");
        Kernel::instance()->terminal().write(int_to_str(current->size));
        Kernel::instance()->terminal().write(" bytes, used=");
        Kernel::instance()->terminal().write(current->used ? "yes\n" : "no\n");

        current = current->next;
    }

    Kernel::instance()->terminal().write("  Total used: ");
    Kernel::instance()->terminal().write(int_to_str(used_memory));
    Kernel::instance()->terminal().write(" bytes\n");
    Kernel::instance()->terminal().write("  Total free: ");
    Kernel::instance()->terminal().write(int_to_str(getFreeMemory()));
    Kernel::instance()->terminal().write(" bytes\n");
}
