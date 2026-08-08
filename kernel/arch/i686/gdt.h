#ifndef KERNEL_GDT_H
#define KERNEL_GDT_H

#include "../../../shared/types.h"

/**
 * @brief GDT and TSS definitions for i686 architecture
 */
#define NULL_SEGMENT 0
#define KERNEL_CODE_SEGMENT 1
#define KERNEL_DATA_SEGMENT 2
#define USER_CODE_SEGMENT 3
#define USER_DATA_SEGMENT 4
#define TSS_SEGMENT 5

/**
 * @brief Access flags for GDT entries
 */
#define ACCESS_KERNEL_CODE 0x9A
#define ACCESS_KERNEL_DATA 0x92
#define ACCESS_USER_CODE 0xFA
#define ACCESS_USER_DATA 0xF2

/**
 * @brief Granularity flags for GDT entries
 */
#define GRANULARITY 0xCF

/// @brief GDT entry structure \struct gdt_entry
struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} PACKED;

/// @brief GDT pointer structure \struct gdt_ptr
struct gdt_ptr
{
    uint16_t limit;
    uint32_t base;
} PACKED;

/// @brief TSS entry structure \struct tss_entry
struct tss_entry
{
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} PACKED;

/**
 * @brief Initialize the GDT
 */
void gdt_init(void);

/**
 * @brief Set a GDT entry
 *
 * @param num The index of the GDT entry
 * @param base The base address
 * @param limit The limit
 * @param access The access flags
 * @param gran The granularity flags
 */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

/**
 * @brief Set the kernel stack for the TSS
 *
 * @param stack The stack pointer
 */
void tss_set_kernel_stack(uint32_t stack);

/**
 * @brief Flush the GDT
 *
 * @param gdt_ptr The pointer to the GDT
 */
extern void gdt_flush(uint32_t);

/**
 * @brief Flush the TSS
 */
extern void tss_flush(void);

#endif
