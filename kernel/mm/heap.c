#include "heap.h"
#include "alloc_track.h"
#include "include/cast.h"
#include "arch/i686/arch.h"
#include "lib/log.h"
#include "ui/console.h"

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
    heap_start = PTR_FROM_U32_TYPED_STRICT(struct heap_block, start);
    heap_size = size;
    heap_used = 0;

    heap_start->size = size - sizeof(struct heap_block);
    heap_start->used = 0;
    heap_start->next = NULL;
    return (uint8_t*)heap_start + sizeof(struct heap_block);
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
    uint32_t best_diff = LIMIT;

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
        log_error_fmt("kmalloc called with size 0");
        return NULL;
    }
    size = (size + 3) & ~3;

    void* result = NULL;
    CRITICAL_SECTION
    {
        struct heap_block* block = find_best_fit(size);
        if (block)
        {
            split_block(block, size);
            block->used = 1;
            heap_used += block->size + sizeof(struct heap_block);
            result = (void*)((uint8_t*)block + sizeof(struct heap_block));
            break;
        }

        merge_free_blocks();

        block = find_best_fit(size);
        if (block)
        {
            split_block(block, size);
            block->used = 1;
            heap_used += block->size + sizeof(struct heap_block);
            result = (void*)((uint8_t*)block + sizeof(struct heap_block));
        }
    }

    if (result)
    {
        TRACK_ADD(result, size, ALLOC_SRC_KMALLOC);
    }
    return result;
}

void* kmalloc_aligned(const size_t size, const size_t align)
{
    if (align == 0 || (align & (align - 1)) != 0)
    {
        log_error_fmt("kmalloc_aligned called with invalid alignment: %zu", align);
        return NULL;
    }

    const size_t total = size + align + sizeof(void*) + sizeof(uint32_t);
    void* ptr = kmalloc(total);
    if (!ptr)
    {
        log_error("kmalloc_aligned ptr not valid");
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

static void kfree_unlocked(void* ptr)
{
    const uint32_t addr = PTR_TO_U32(ptr);
    const uint32_t heap_start_addr = PTR_TO_U32(heap_start);
    const uint32_t heap_end_addr = heap_start_addr + heap_size;

    if (addr < heap_start_addr || addr >= heap_end_addr)
    {
        return;
    }

    struct heap_block* block = (struct heap_block*)((uint8_t*)ptr - sizeof(struct heap_block));
    if (block->used)
    {
        const uint32_t to_sub = block->size + sizeof(struct heap_block);
        if (to_sub <= heap_used)
        {
            heap_used -= to_sub;
        }
        else
        {
            heap_used = 0;
            log_error_fmt("heap_used underflow on free at %p", ptr);
        }
        block->used = 0;
        merge_free_blocks();
        heap_validate();
    }
}

void kfree(void* ptr)
{
    if (!ptr)
    {
        log_error_fmt("Attempted to free a NULL pointer");
        return;
    }

    if (alloc_track_is_initialized())
    {
        TRACK_REMOVE(ptr, ALLOC_SRC_KMALLOC);
    }

    CRITICAL_SECTION { kfree_unlocked(ptr); };
}
void kfree_aligned(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    CRITICAL_SECTION
    {
        const uint32_t addr = PTR_TO_U32(ptr);
        const uint32_t heap_start_addr = PTR_TO_U32(heap_start);
        const uint32_t heap_end_addr = heap_start_addr + heap_size;

        if (addr < heap_start_addr || addr >= heap_end_addr)
        {
            break;
        }

        const uint32_t* magic = (const uint32_t*)((uint8_t*)ptr - sizeof(void*) - sizeof(uint32_t));
        if (*magic != 0xA11C4FED)
        {
            log_error_fmt("Invalid magic number for aligned free at %p", ptr);
            break;
        }

        void* orig_ptr = *(void**)((uint8_t*)ptr - sizeof(void*));

        if (PTR_TO_U32(orig_ptr) < heap_start_addr || PTR_TO_U32(orig_ptr) >= heap_end_addr)
        {
            log_error_fmt("Original pointer for aligned free at %p is out of heap bounds", orig_ptr);
            break;
        }

        TRACK_REMOVE(orig_ptr, ALLOC_SRC_KMALLOC);
        kfree_unlocked(orig_ptr);
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

void heap_shutdown(void)
{
    heap_start = NULL;
    heap_size = 0;
    heap_used = 0;
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: heap driver shutdown complete\n", __FUNCTION__);
    console_write(msg);
}
