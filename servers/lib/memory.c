#include "memory.h"
#include "../../user/lib/syscall.h"

/**
 * @brief Heap block header structure \struct heap_block
 */
struct heap_block
{
    uint32_t size;
    uint32_t magic;
    struct heap_block *next;
    uint8_t free;
    uint8_t padding[3];
};

/** Magic number for heap validation */
#define HEAP_MAGIC 0xDEADBEEF

/** Minimum block size */
#define MIN_BLOCK_SIZE (sizeof(struct heap_block) + 16)

/**
 * @brief Heap state structure \struct heap_state
 */
static struct
{
    void *base;
    uint32_t size;
    struct heap_block *free_list;
    uint32_t total_allocated;
    uint32_t total_free;
    bool initialized;
} heap_state = {0};

int mem_init(void *heap_base, uint32_t heap_size)
{
    if (heap_base == NULL || heap_size < MIN_BLOCK_SIZE)
    {
        return -1;
    }

    heap_state.base = heap_base;
    heap_state.size = heap_size;

    /* Initialize first free block spanning entire heap */
    struct heap_block *first = (struct heap_block *)heap_base;
    first->size = heap_size;
    first->magic = HEAP_MAGIC;
    first->next = NULL;
    first->free = 1;

    heap_state.free_list = first;
    heap_state.total_allocated = 0;
    heap_state.total_free = heap_size - sizeof(struct heap_block);
    heap_state.initialized = true;

    return 0;
}

void *mem_alloc(uint32_t size)
{
    if (!heap_state.initialized || size == 0)
    {
        return NULL;
    }

    /* Align size to 8 bytes */
    size = (size + 7) & ~7U;
    uint32_t total_size = size + sizeof(struct heap_block);

    /* First-fit search */
    struct heap_block *prev = NULL;
    struct heap_block *current = heap_state.free_list;

    while (current != NULL)
    {
        if (current->magic != HEAP_MAGIC)
        {
            /* Heap corruption detected */
            return NULL;
        }

        if (current->free && current->size >= total_size)
        {
            /* Found a suitable block */
            if (current->size >= total_size + MIN_BLOCK_SIZE)
            {
                /* Split the block */
                struct heap_block *new_block = (struct heap_block *)
                        ((uint8_t *)current + total_size);
                new_block->size = current->size - total_size;
                new_block->magic = HEAP_MAGIC;
                new_block->next = current->next;
                new_block->free = 1;

                current->size = total_size;
                current->next = new_block;
            }

            /* Mark as allocated */
            current->free = 0;

            /* Update free list */
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                heap_state.free_list = current->next;
            }

            heap_state.total_allocated += current->size;
            heap_state.total_free -= current->size;

            /* Return pointer after header */
            return (void *)((uint8_t *)current + sizeof(struct heap_block));
        }

        prev = current;
        current = current->next;
    }

    /* No suitable block found */
    return NULL;
}

void *mem_alloc_aligned(uint32_t size, uint32_t alignment)
{
    if (!heap_state.initialized || size == 0 || alignment == 0)
    {
        return NULL;
    }

    /* Alignment must be power of 2 */
    if ((alignment & (alignment - 1)) != 0)
    {
        return NULL;
    }

    /* Allocate extra space for alignment */
    uint32_t alloc_size = size + alignment + sizeof(void *);
    void *raw = mem_alloc(alloc_size);
    if (raw == NULL)
    {
        return NULL;
    }

    /* Calculate aligned address */
    uint32_t addr = (uint32_t)(uintptr_t)raw;
    uint32_t aligned = (addr + alignment - 1 + sizeof(void *)) & ~(alignment - 1);

    /* Store original pointer before aligned address */
    ((void **)aligned)[-1] = raw;

    return (void *)(uintptr_t)aligned;
}

void mem_free(void *ptr)
{
    if (!heap_state.initialized || ptr == NULL)
    {
        return;
    }

    /* Get block header */
    struct heap_block *block = (struct heap_block *)
            ((uint8_t *)ptr - sizeof(struct heap_block));

    if (block->magic != HEAP_MAGIC || block->free)
    {
        /* Invalid or already free */
        return;
    }

    block->free = 1;

    heap_state.total_allocated -= block->size;
    heap_state.total_free += block->size;

    /* Add to free list (sorted by address) */
    struct heap_block *prev = NULL;
    struct heap_block *current = heap_state.free_list;

    while (current != NULL && current < block)
    {
        prev = current;
        current = current->next;
    }

    block->next = current;
    if (prev)
    {
        prev->next = block;
    }
    else
    {
        heap_state.free_list = block;
    }

    /* Coalesce with next block */
    if (current != NULL &&
        (uint8_t *)block + block->size == (uint8_t *)current)
    {
        block->size += current->size;
        block->next = current->next;
    }

    /* Coalesce with previous block */
    if (prev != NULL && prev->free &&
        (uint8_t *)prev + prev->size == (uint8_t *)block)
    {
        prev->size += block->size;
        prev->next = block->next;
    }
}

void *mem_realloc(void *ptr, uint32_t size)
{
    if (ptr == NULL)
    {
        return mem_alloc(size);
    }

    if (size == 0)
    {
        mem_free(ptr);
        return NULL;
    }

    struct heap_block *block = (struct heap_block *)
            ((uint8_t *)ptr - sizeof(struct heap_block));

    if (block->magic != HEAP_MAGIC)
    {
        return NULL;
    }

    uint32_t old_size = block->size - sizeof(struct heap_block);
    if (size <= old_size)
    {
        /* Shrinking - just return existing block */
        return ptr;
    }

    /* Need to allocate new block */
    void *new_ptr = mem_alloc(size);
    if (new_ptr == NULL)
    {
        return NULL;
    }

    mem_copy(new_ptr, ptr, old_size);
    mem_free(ptr);

    return new_ptr;
}

void *mem_calloc(uint32_t count, uint32_t size)
{
    uint32_t total = count * size;
    void *ptr = mem_alloc(total);
    if (ptr != NULL)
    {
        mem_set(ptr, 0, total);
    }
    return ptr;
}

void *mem_map_phys(uint32_t phys_addr, uint32_t size, uint32_t prot, uint32_t flags)
{
    uint32_t syscall_flags = 0;
    if (flags & MEM_FLAG_DEVICE)
    {
        syscall_flags |= 0x04;
    }
    (void)prot;
    return sys_map_device(phys_addr, size, syscall_flags);
}

int mem_unmap(void *addr, uint32_t size)
{
    /* TODO: Implement via munmap syscall */
    (void)addr;
    (void)size;
    return -1;
}

int mem_share_create(uint32_t size, const char *name)
{
    /* TODO: Implement shared memory */
    (void)size;
    (void)name;
    return -1;
}

void *mem_share_attach(int handle)
{
    /* TODO: Implement shared memory */
    (void)handle;
    return NULL;
}

int mem_share_detach(void *ptr)
{
    /* TODO: Implement shared memory */
    (void)ptr;
    return -1;
}

void mem_stats(uint32_t *total_bytes, uint32_t *used_bytes, uint32_t *free_bytes)
{
    if (total_bytes)
    {
        *total_bytes = heap_state.size;
    }
    if (used_bytes)
    {
        *used_bytes = heap_state.total_allocated;
    }
    if (free_bytes)
    {
        *free_bytes = heap_state.total_free;
    }
}

void *mem_copy(void *dest, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    while (n--)
    {
        *d++ = *s++;
    }

    return dest;
}

void *mem_set(void *dest, int val, uint32_t n)
{
    uint8_t *d = (uint8_t *)dest;
    uint8_t v = (uint8_t)val;

    while (n--)
    {
        *d++ = v;
    }

    return dest;
}

int mem_cmp(const void *s1, const void *s2, uint32_t n)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    while (n--)
    {
        if (*p1 != *p2)
        {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }

    return 0;
}
