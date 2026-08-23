#include "shm.h"

#include "addr.h"
#include "sched/sched.h"
#include "mm/page.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "lib/string.h"

#define SHM_MAX_OBJECTS 16
#define SHM_MAX_MAPPINGS 32

/**
 * @brief Structure representing a shared memory object. \struct shm_object
 */
struct shm_object
{
    bool used;
    bool destroy_pending;
    pid_t owner;
    uint32_t physical;
    uint32_t pages;
    uint32_t size;
    uint32_t references;
};

/**
 * @brief Structure representing a mapping of a shared memory object to a process. \struct shm_mapping
 */
struct shm_mapping
{
    bool used;
    pid_t pid;
    int object;
    uint32_t address;
};

static struct shm_object objects[SHM_MAX_OBJECTS];
static struct shm_mapping mappings[SHM_MAX_MAPPINGS];

void shm_init(void)
{
    memset(objects, 0, sizeof(objects));
    memset(mappings, 0, sizeof(mappings));
}

int shm_create(const uint32_t size, const pid_t owner)
{
    if (!size || size > 4U * 1024U * 1024U) return -1;
    const uint32_t pages = (size + PAGE_SIZE - 1U) / PAGE_SIZE;
    for (int i = 0; i < SHM_MAX_OBJECTS; i++)
    {
        if (objects[i].used) continue;
        void* physical = pmm_alloc_blocks(pages);
        if (!physical) return -1;
        memset(phys_to_virt(PTR_TO_U32(physical)), 0, pages * PAGE_SIZE);
        objects[i].used = true;
        objects[i].owner = owner;
        objects[i].physical = (uint32_t)(uintptr_t)physical;
        objects[i].pages = pages;
        objects[i].size = size;
        return i;
    }
    return -1;
}

uint32_t shm_map(const int id, const pid_t pid)
{
    struct task* task = task_find(pid);
    if (!task || id < 0 || id >= SHM_MAX_OBJECTS || !objects[id].used) return 0;
    int mapping_slot = -1;
    for (int i = 0; i < SHM_MAX_MAPPINGS; i++)
    {
        if (!mappings[i].used)
        {
            mapping_slot = i;
            break;
        }
    }
    if (mapping_slot < 0) return 0;

    const uint32_t bytes = objects[id].pages * PAGE_SIZE;
    if (task->heap_next > USER_HEAP_LIMIT || bytes > USER_HEAP_LIMIT - task->heap_next) return 0;
    const uint32_t address = task->heap_next;
    page_directory_t* directory = (page_directory_t*)(uintptr_t)task->context.cr3;
    for (uint32_t page = 0; page < objects[id].pages; page++)
    {
        if (vmm_map_page(directory, address + page * PAGE_SIZE,
                         objects[id].physical + page * PAGE_SIZE,
                         PAGE_PRESENT | PAGE_WRITE | PAGE_USER | PAGE_SHARED) != 0)
        {
            for (uint32_t rollback = 0; rollback < page; rollback++)
            {
                vmm_unmap_page(directory, address + rollback * PAGE_SIZE);
            }
            return 0;
        }
    }
    task->heap_next += bytes;
    mappings[mapping_slot].used = true;
    mappings[mapping_slot].pid = pid;
    mappings[mapping_slot].object = id;
    mappings[mapping_slot].address = address;
    objects[id].references++;
    return address;
}

static void maybe_free(const int id)
{
    if (objects[id].used && objects[id].destroy_pending && objects[id].references == 0)
    {
        pmm_free_blocks((void*)(uintptr_t)objects[id].physical, objects[id].pages);
        memset(&objects[id], 0, sizeof(objects[id]));
    }
}

int shm_detach(const int id, const pid_t pid)
{
    struct task* task = task_find(pid);
    if (!task) return -1;
    for (int i = 0; i < SHM_MAX_MAPPINGS; i++)
    {
        if (!mappings[i].used || mappings[i].pid != pid || mappings[i].object != id) continue;
        for (uint32_t page = 0; page < objects[id].pages; page++)
        {
            vmm_unmap_page((page_directory_t*)(uintptr_t)task->context.cr3,mappings[i].address + page * PAGE_SIZE);
        }
        mappings[i].used = false;
        if (objects[id].references) objects[id].references--;
        maybe_free(id);
        return 0;
    }
    return -1;
}

int shm_destroy(const int id, const pid_t owner)
{
    if (id < 0 || id >= SHM_MAX_OBJECTS || !objects[id].used || objects[id].owner != owner) return -1;
    objects[id].destroy_pending = true;
    maybe_free(id);
    return 0;
}

void shm_process_cleanup(const pid_t pid)
{
    for (int i = 0; i < SHM_MAX_MAPPINGS; i++)
    {
        if (mappings[i].used && mappings[i].pid == pid)
        {
            const int id = mappings[i].object;
            mappings[i].used = false;
            if (objects[id].references) objects[id].references--;
            maybe_free(id);
        }
    }
    for (int i = 0; i < SHM_MAX_OBJECTS; i++)
    {
        if (objects[i].used && objects[i].owner == pid)
        {
            objects[i].destroy_pending = true;
            maybe_free(i);
        }
    }
}

void shm_process_fork(const pid_t parent, const pid_t child)
{
    for (int i = 0; i < SHM_MAX_MAPPINGS; i++)
    {
        if (!mappings[i].used || mappings[i].pid != parent) continue;
        for (int j = 0; j < SHM_MAX_MAPPINGS; j++)
        {
            if (!mappings[j].used)
            {
                mappings[j] = mappings[i];
                mappings[j].pid = child;
                objects[mappings[j].object].references++;
                break;
            }
        }
    }
}
