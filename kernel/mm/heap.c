#include "heap.h"
#include "../../shared/string.h"
#include "../include/cast.h"

/// @brief Heap block structure \struct heap_block
struct heap_block
{
    uint32_t size;
    uint8_t  used;
    struct heap_block* next;
};

/**
 * @brief Heap management variables
 */
static struct heap_block* heap_start = NULL;
static uint32_t heap_size = 0;
static uint32_t heap_used = 0;

void* heap_init(const uint32_t start, const uint32_t size)
{
    heap_start = PTR_FROM_U32_TYPED(struct heap_block, start);
    heap_size = size;
    heap_used = sizeof(struct heap_block);

    heap_start->size = size - sizeof(struct heap_block);
    heap_start->used = 0;
    heap_start->next = NULL;
    return (void*)((uint8_t*)heap_start + sizeof(struct heap_block));
}

static void split_block(struct heap_block* block, const uint32_t size)
{
    if (block->size >= size + sizeof(struct heap_block) + 16)
    {
        struct heap_block* new_block = (struct heap_block*)((uint8_t*)block + sizeof(struct heap_block) + size);
        new_block->size = block->size - size - sizeof(struct heap_block);
        new_block->used = 0;
        new_block->next = block->next;
        block->size = size;
        block->next = new_block;
    }
}

static void merge_free_blocks(void)
{
    struct heap_block* block = heap_start;
    const uint8_t* heap_end = (uint8_t*)heap_start + heap_size;

    while (block && block->next)
    {
        if ((uint8_t*)block < (uint8_t*)heap_start || (uint8_t*)block >= heap_end)
        {
            break;
        }
        if ((uint8_t*)block->next < (uint8_t*)heap_start || (uint8_t*)block->next >= heap_end)
        {
            break;
        }

        if (!block->used && !block->next->used)
        {
            const uint32_t new_size = block->size + sizeof(struct heap_block) + block->next->size;
            if (new_size < block->size || new_size >= heap_size)
            {
                block = block->next;
                continue;
            }

            block->size = new_size;
            block->next = block->next->next;
        }
        else
        {
            block = block->next;
        }
    }
}

static struct heap_block* find_best_fit(const size_t size)
{
    struct heap_block* best = NULL;
    struct heap_block* block = heap_start;
    uint32_t best_diff = 0xFFFFFFFF;

    while (block)
    {
        if (!block->used && block->size >= size)
        {
            const uint32_t diff = block->size - size;
            if (diff < best_diff)
            {
                best = block;
                best_diff = diff;
                if (diff == 0)
                {
                    break;
                }
            }
        }
        block = block->next;
    }
    return best;
}

void* kmalloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    size = (size + 3) & ~3;

    struct heap_block* block = find_best_fit(size);
    if (block)
    {
        split_block(block, size);
        block->used = 1;
        heap_used += size + sizeof(struct heap_block);
        return (void*)((uint8_t*)block + sizeof(struct heap_block));
    }

    merge_free_blocks();

    block = find_best_fit(size);
    if (block)
    {
        split_block(block, size);
        block->used = 1;
        heap_used += size + sizeof(struct heap_block);
        return (void*)((uint8_t*)block + sizeof(struct heap_block));
    }

    return NULL;
}

void* kmalloc_aligned(const size_t size, const size_t align)
{
    if (align == 0 || (align & (align - 1)) != 0)
    {
        return NULL;
    }

    const size_t total = size + align + sizeof(void*) + sizeof(uint32_t);
    void* ptr = kmalloc(total);
    if (!ptr)
    {
        return NULL;
    }

    const uint32_t addr = PTR_TO_U32(ptr);
    const uint32_t aligned = (addr + sizeof(void*) + sizeof(uint32_t) + align - 1) & ~(align - 1);

    uint32_t* magic = (uint32_t*)(aligned - sizeof(void*) - sizeof(uint32_t));
    void** orig_ptr = (void**)(aligned - sizeof(void*));
    *magic = 0xA11C4FED;
    *orig_ptr = ptr;

    return PTR_FROM_U32(aligned);
}

static void heap_validate(void)
{
    struct heap_block* block = heap_start;
    const uint8_t* heap_end = (uint8_t*)heap_start + heap_size;

    while (block)
    {
        if ((uint8_t*)block < (uint8_t*)heap_start || (uint8_t*)block >= heap_end)
        {
            break;
        }

        if (!block->used && block->size == 0 && block->next != NULL)
        {
            merge_free_blocks();
            return;
        }

        block = block->next;
    }
}

void kfree(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    const uint32_t addr = PTR_TO_U32(ptr);
    const uint32_t heap_start_addr = PTR_TO_U32(heap_start);
    const uint32_t heap_end_addr = heap_start_addr + heap_size;

    if (addr < heap_start_addr || addr >= heap_end_addr)
    {
        return;
    }

    const uint32_t* magic_location = (uint32_t*)((uint8_t*)ptr - sizeof(void*) - sizeof(uint32_t));
    if (*magic_location == 0xA11C4FED)
    {
        void** orig_ptr_location = (void**)((uint8_t*)ptr - sizeof(void*));
        void* orig_ptr = *orig_ptr_location;

        if (PTR_TO_U32(orig_ptr) < heap_start_addr || PTR_TO_U32(orig_ptr) >= heap_end_addr)
        {
            return;
        }

        ptr = orig_ptr;
    }

    struct heap_block* block = (struct heap_block*)((uint8_t*)ptr - sizeof(struct heap_block));

    if (block->used)
    {
        heap_used -= block->size + sizeof(struct heap_block);
        block->used = 0;
        merge_free_blocks();
        heap_validate();
    }
}

size_t heap_get_used(void)
{
    return heap_used;
}

size_t heap_get_free(void)
{
    return heap_size - heap_used;
}

void heap_get_fragmentation(uint32_t* free_blocks, uint32_t* largest_free)
{
    uint32_t count = 0;
    uint32_t largest = 0;

    struct heap_block* block = heap_start;
    const uint8_t* heap_end = (uint8_t*)heap_start + heap_size;
    uint32_t iterations = 0;
    const uint32_t max_iterations = 10000;

    while (block && iterations < max_iterations)
    {
        if ((uint8_t*)block < (uint8_t*)heap_start || (uint8_t*)block >= heap_end)
        {
            break;
        }

        if (block->size == 0 && block->next != NULL)
        {
            merge_free_blocks();
            block = heap_start;
            count = 0;
            largest = 0;
            continue;
        }

        if (!block->used)
        {
            count++;
            if (block->size > largest)
            {
                largest = block->size;
            }
        }
        block = block->next;
        iterations++;
    }

    if (free_blocks)
    {
        *free_blocks = count;
    }
    if (largest_free)
    {
        *largest_free = largest;
    }
}

void heap_defragment(void)
{
    merge_free_blocks();
}
