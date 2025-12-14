#include "initrd.h"
#include "../shared/string.h"

extern struct initrd_entry __initrd_start[];
extern struct initrd_entry __initrd_end[];

size_t initrd_num_entries(void)
{
    return (size_t)(__initrd_end - __initrd_start);
}

const struct initrd_entry* initrd_get_entry(size_t idx)
{
    size_t count = initrd_num_entries();
    if (idx >= count) return NULL;
    return &__initrd_start[idx];
}

const struct initrd_entry* initrd_find(const char* name)
{
    for (size_t i = 0; i < initrd_num_entries(); i++)
    {
        const struct initrd_entry* e = &__initrd_start[i];
        if (e->name && name && strcmp(e->name, name) == 0)
        {
            return e;
        }
    }
    return NULL;
}
