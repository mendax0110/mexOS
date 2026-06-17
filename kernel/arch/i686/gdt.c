#include "gdt.h"
#include "../include/string.h"
#include "../include/config.h"
#include "../include/cast.h"

static struct gdt_entry gdt_entries[6];
static struct gdt_ptr   gdt_pointer;
static struct tss_entry tss;

void gdt_set_gate(const int num, const uint32_t base, const uint32_t limit, const uint8_t access, const uint8_t gran)
{
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_mid    = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;
    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

static void tss_write(const int num, const uint32_t ss0, const uint32_t esp0)
{
    const uint32_t base  = PTR_TO_U32(&tss);
    const uint32_t limit = sizeof(tss) - 1;

    gdt_set_gate(num, base, limit, 0xE9, 0x00);
    memset(&tss, 0, sizeof(tss));
    tss.ss0  = ss0;
    tss.esp0 = esp0;
    tss.cs   = KERNEL_CS;
    tss.ss   = tss.ds = tss.es = tss.fs = tss.gs = KERNEL_DS;
    tss.iomap_base = sizeof(tss);
}

void gdt_init(void)
{
    gdt_pointer.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdt_pointer.base = PTR_TO_U32(gdt_entries);

    gdt_set_gate(NULL_SEGMENT, NULL_SEGMENT, NULL_SEGMENT, NULL_SEGMENT, NULL_SEGMENT);                // Null segment
    gdt_set_gate(KERNEL_CODE_SEGMENT, NULL_SEGMENT, LIMIT, ACCESS_KERNEL_CODE, GRANULARITY); // Kernel code
    gdt_set_gate(KERNEL_DATA_SEGMENT, NULL_SEGMENT, LIMIT, ACCESS_KERNEL_DATA, GRANULARITY); // Kernel data
    gdt_set_gate(USER_CODE_SEGMENT, NULL_SEGMENT, LIMIT, ACCESS_USER_CODE, GRANULARITY); // User code
    gdt_set_gate(USER_DATA_SEGMENT, NULL_SEGMENT, LIMIT, ACCESS_USER_DATA, GRANULARITY); // User data
    tss_write(TSS_SEGMENT, KERNEL_DS, NULL_SEGMENT);                  // TSS

    gdt_flush(PTR_TO_U32(&gdt_pointer));
    tss_flush();
}

void tss_set_kernel_stack(const uint32_t stack)
{
    tss.esp0 = stack;
}
