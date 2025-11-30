#include "gdt.h"
#include "../include/string.h"
#include "../include/config.h"

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
    const uint32_t base = (uint32_t)&tss;
    const uint32_t limit = base + sizeof(tss);

    gdt_set_gate(num, base, limit, 0xE9, 0x00);
    memset(&tss, 0, sizeof(tss));
    tss.ss0  = ss0;
    tss.esp0 = esp0;
    tss.cs   = KERNEL_CS | 0x3;
    tss.ss   = tss.ds = tss.es = tss.fs = tss.gs = KERNEL_DS | 0x3;
    tss.iomap_base = sizeof(tss);
}

void gdt_init(void)
{
    gdt_pointer.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdt_pointer.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel code
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel data
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User code
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User data
    tss_write(5, KERNEL_DS, 0);                  // TSS

    gdt_flush((uint32_t)&gdt_pointer);
    tss_flush();
}

void tss_set_kernel_stack(const uint32_t stack)
{
    tss.esp0 = stack;
}
