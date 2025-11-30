#include "pmm.h"
#include "../include/string.h"

/**
 * @brief Physical Memory Manager (PMM) constants
 */
#define PMM_BLOCK_SIZE   4096
#define PMM_BLOCKS_PER_BYTE 8

/**
 * @brief Physical Memory Manager (PMM) variables
 */
static uint32_t* pmm_bitmap = 0;
static uint32_t pmm_bitmap_size = 0;
static uint32_t pmm_memory_size = 0;
static uint32_t pmm_used_blocks = 0;
static uint32_t pmm_max_blocks = 0;

static inline void bitmap_set(const uint32_t bit)
{
    pmm_bitmap[bit / 32] |= (1 << (bit % 32));
}

static inline void bitmap_unset(const uint32_t bit)
{
    pmm_bitmap[bit / 32] &= ~(1 << (bit % 32));
}

static inline int bitmap_test(const uint32_t bit)
{
    return pmm_bitmap[bit / 32] & (1 << (bit % 32));
}

static int bitmap_first_free(void)
{
    for (uint32_t i = 0; i < pmm_max_blocks / 32; i++)
    {
        if (pmm_bitmap[i] != 0xFFFFFFFF)
        {
            for (int j = 0; j < 32; j++)
            {
                const int bit = 1 << j;
                if (!(pmm_bitmap[i] & bit))
                {
                    return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

static int bitmap_first_free_s(const uint32_t size)
{
    if (size == 0) return -1;
    if (size == 1) return bitmap_first_free();

    for (uint32_t i = 0; i < pmm_max_blocks / 32; i++)
    {
        if (pmm_bitmap[i] != 0xFFFFFFFF)
        {
            for (int j = 0; j < 32; j++)
            {
                const int bit = 1 << j;
                if (!(pmm_bitmap[i] & bit))
                {
                    const int start = i * 32 + j;
                    uint32_t free = 0;
                    for (uint32_t k = 0; k < size; k++)
                    {
                        if (!bitmap_test(start + k)) free++;
                        if (free == size) return start;
                    }
                }
            }
        }
    }
    return -1;
}

void pmm_init(const uint32_t mem_size, const uint32_t bitmap_addr)
{
    pmm_memory_size = mem_size;
    pmm_bitmap = (uint32_t*)bitmap_addr;
    pmm_max_blocks = mem_size / PMM_BLOCK_SIZE;
    pmm_bitmap_size = pmm_max_blocks / PMM_BLOCKS_PER_BYTE;
    pmm_used_blocks = pmm_max_blocks;
    memset(pmm_bitmap, 0xFF, pmm_bitmap_size);
}

void pmm_init_region(const uint32_t base, const uint32_t size)
{
    int align = base / PMM_BLOCK_SIZE;
    int blocks = size / PMM_BLOCK_SIZE;

    for (; blocks > 0; blocks--)
    {
        bitmap_unset(align++);
        pmm_used_blocks--;
    }
    bitmap_set(0);  // nullptr protection
}

void pmm_deinit_region(const uint32_t base, const uint32_t size)
{
    int align = base / PMM_BLOCK_SIZE;
    int blocks = size / PMM_BLOCK_SIZE;

    for (; blocks > 0; blocks--)
    {
        bitmap_set(align++);
        pmm_used_blocks++;
    }
}

void* pmm_alloc_block(void)
{
    if (pmm_get_free_block_count() == 0) return 0;

    const int frame = bitmap_first_free();
    if (frame == -1) return 0;

    bitmap_set(frame);
    pmm_used_blocks++;

    return (void*)(frame * PMM_BLOCK_SIZE);
}

void pmm_free_block(void* p)
{
    const uint32_t addr = (uint32_t)p;
    const int frame = addr / PMM_BLOCK_SIZE;

    bitmap_unset(frame);
    pmm_used_blocks--;
}

void* pmm_alloc_blocks(const uint32_t count)
{
    if (pmm_get_free_block_count() < count) return 0;

    const int frame = bitmap_first_free_s(count);
    if (frame == -1) return 0;

    for (uint32_t i = 0; i < count; i++)
    {
        bitmap_set(frame + i);
    }
    pmm_used_blocks += count;

    return (void*)(frame * PMM_BLOCK_SIZE);
}

void pmm_free_blocks(void* p, const uint32_t count)
{
    const uint32_t addr = (uint32_t)p;
    const int frame = addr / PMM_BLOCK_SIZE;

    for (uint32_t i = 0; i < count; i++)
    {
        bitmap_unset(frame + i);
    }
    pmm_used_blocks -= count;
}

uint32_t pmm_get_memory_size(void) { return pmm_memory_size; }
uint32_t pmm_get_block_count(void) { return pmm_max_blocks; }
uint32_t pmm_get_used_block_count(void) { return pmm_used_blocks; }
uint32_t pmm_get_free_block_count(void) { return pmm_max_blocks - pmm_used_blocks; }
