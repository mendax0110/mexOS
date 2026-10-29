#ifndef KERNEL_PAGE_H
#define KERNEL_PAGE_H

#include "../arch/i686/boot_defines.h"
#include "../../shared/types.h"

typedef uint32_t page_directory_t[PAGE_DIRECTORY_ENTRIES] ALIGNED(4096);
typedef uint32_t page_table_t[PAGE_DIRECTORY_ENTRIES] ALIGNED(4096);

#endif // KERNEL_PAGE_H
