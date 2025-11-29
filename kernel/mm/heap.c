#include "heap.h"
#include "../include/string.h"

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

void heap_init(uint32_t start, uint32_t size)
{
    heap_start = (struct heap_block*)start;
    heap_size = size;
    heap_used = sizeof(struct heap_block);

    heap_start->size = size - sizeof(struct heap_block);
    heap_start->used = 0;
    heap_start->next = NULL;
}

static void split_block(struct heap_block* block, uint32_t size)
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
    while (block && block->next)
    {
        if (!block->used && !block->next->used)
        {
            block->size += sizeof(struct heap_block) + block->next->size;
            block->next = block->next->next;
        }
        else
        {
            block = block->next;
        }
    }
}

void* kmalloc(size_t size)
{
    if (size == 0) return NULL;
    size = (size + 3) & ~3;  // Align to 4 bytes

    struct heap_block* block = heap_start;
    while (block)
    {
        if (!block->used && block->size >= size)
        {
            split_block(block, size);
            block->used = 1;
            heap_used += size + sizeof(struct heap_block);
            return (void*)((uint8_t*)block + sizeof(struct heap_block));
        }
        block = block->next;
    }
    return NULL;
}

void* kmalloc_aligned(size_t size, size_t align)
{
    size_t total = size + align;
    void* ptr = kmalloc(total);
    if (!ptr) return NULL;
    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned = (addr + align - 1) & ~(align - 1);
    return (void*)aligned;
}

void kfree(void* ptr)
{
    if (!ptr) return;

    struct heap_block* block = (struct heap_block*)((uint8_t*)ptr - sizeof(struct heap_block));
    if (block->used)
    {
        heap_used -= block->size + sizeof(struct heap_block);
        block->used = 0;
        merge_free_blocks();
    }
}

size_t heap_get_used(void) { return heap_used; }
size_t heap_get_free(void) { return heap_size - heap_used; }
