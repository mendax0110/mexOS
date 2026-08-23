#ifndef KERNEL_PAGE_H
#define KERNEL_PAGE_H

#include "../../shared/types.h"

#define PAGE_SIZE 0x1000

#define KERNEL_VIRTUAL_BASE 0xC0000000
#define USER_SPACE_END 0xBFFFFFFF

#define KERNEL_PAGE_NUMBER   (KERNEL_VIRTUAL_BASE >> 22)   // 768 PDE index is wherea kernel begins...
#define KERNEL_DIRECT_MAP_MB 128

#define KERNEL_MMIO_VIRT_BASE (KERNEL_VIRTUAL_BASE + KERNEL_DIRECT_MAP_MB * 1024U * 1024U)
#define KERNEL_MMIO_VIRT_SIZE 0x08000000U

#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FF)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*(x) & ~0xFFF)

#define PAGE_PRESENT       0x001
#define PAGE_WRITE         0x002
#define PAGE_USER          0x004
#define PAGE_WRITETHROUGH  0x008
#define PAGE_CACHE_DISABLE 0x010
#define PAGE_ACCESSED      0x020
#define PAGE_DIRTY         0x040
#define PAGE_SIZE_BIT      0x080
#define PAGE_GLOBAL        0x100
#define PAGE_SHARED        0x200

#define PAGE_DIRECTORY_ENTRIES 1024
#define USER_SPACE_ENTRIES 768
#define KERNEL_SPACE_ENTRIES 256

typedef uint32_t page_directory_t[PAGE_DIRECTORY_ENTRIES] ALIGNED(4096);
typedef uint32_t page_table_t[PAGE_DIRECTORY_ENTRIES] ALIGNED(4096);

#endif // KERNEL_PAGE_H
