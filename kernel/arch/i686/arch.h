#ifndef ARCH_I686_H
#define ARCH_I686_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Save and disable interrupts, restore on scope exit.
 * Usage:  CRITICAL_SECTION { ... }
 */
static uint32_t irq_save(void)
{
    uint32_t flags;
    __asm__ volatile ("pushfl; popl %0; cli" : "=r"(flags));
    return flags;
}

/**
 * @brief Restore interrupts to previous state
 * @param flags The saved EFLAGS value to restore
 */
static void irq_restore(uint32_t flags)
{
    __asm__ volatile ("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
}

/**
 * @brief Restore interrupts using a pointer to saved flags
 * @param flags Pointer to the saved EFLAGS value to restore
 */
static void irq_restore_ptr(uint32_t* flags)
{
    irq_restore(*flags);
}

/**
 * @brief Save and disable interrupts, restore on scope exit.
 * Usage:  CRITICAL_SECTION { ... }
 *
 * Implemented as two nested for-loops so that a `break` inside the body
 * exits the inner loop while the outer loop's post-expression still runs
 * irq_restore(). A single-loop version would skip the post-expression on
 * `break`, leaving interrupts permanently disabled.
 *
 * Note: `return`/`goto` out of the block still bypasses the restore — use
 * `break` to leave a CRITICAL_SECTION early.
 */
#define CRITICAL_SECTION                                    \
    for (uint32_t _irq_flags_ = irq_save(), _irq_once_ = 1; \
         _irq_once_;                                        \
         irq_restore(_irq_flags_), _irq_once_ = 0)          \
        for (; _irq_once_; _irq_once_ = 0)

/*#define CRITICAL_SECTION \
    for (uint32_t _irq_flags_ = __attribute__((cleanup(irq_restore_ptr))) \
            irq_save(), _irq_once_ = 1; \
            _irq_once_; \
            _irq_once_ = 0)*/

/**
 * @brief Write a byte to the specified port
 * @param port The port to write to
 * @param val A The byte value to write
 */
static void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * @brief Read a byte from the specified port
 * @param port The port to read from
 * @return The byte value read from the port
 */
static uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Write a word to the specified port
 * @param port The port to write to
 * @param val The word value to write
 */
static void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * @brief Read a word from the specified port
 * @param port The port to read from
 * @return The word value read from the port
 */
static uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Write a double word to the specified port
 * @param port The port to write to
 * @param val The double word value to write
 */
static void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * @brief Read a double word from the specified port
 * @param port The port to read from
 * @return The double word value read from the port
 */
static uint32_t inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief I/O wait by writing to an unused port
 */
static void io_wait(void)
{
    outb(0x80, 0);
}

/**
 * @brief Clear interrupts
 */
static void cli(void) { __asm__ volatile ("cli"); }

/**
 * @brief Set interrupts
 */
static void sti(void) { __asm__ volatile ("sti"); }

/**
 * @brief Halt CPU until next interrupt
 */
static void hlt(void) { __asm__ volatile ("hlt"); }

/**
 * @brief Read and write EFLAGS and control registers
 */
static uint32_t read_eflags(void)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

/**
 * @brief Write to EFLAGS register
 * @param eflags The value to write to EFLAGS
 */
static void write_eflags(uint32_t eflags)
{
    __asm__ volatile ("pushl %0; popfl" : : "r"(eflags));
}

/**
 * @brief Read and write control registers CR0, CR2, and CR3
 */
static uint32_t read_cr0(void)
{
    uint32_t val;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(val));
    return val;
}

/**
 * @brief Write to control register CR0
 * @param val The value to write to CR0
 */
static void write_cr0(uint32_t val)
{
    __asm__ volatile ("mov %0, %%cr0" : : "r"(val));
}

/**
 * @brief Read control register CR2
 * @return The value of CR2
 */
static uint32_t read_cr2(void)
{
    uint32_t val;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(val));
    return val;
}

/**
 * @brief Read and write control register CR3
 */
static uint32_t read_cr3(void)
{
    uint32_t val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

/**
 * @brief Write to control register CR3
 * @param val The value to write to CR3
 */
static void write_cr3(uint32_t val)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(val));
}

/**
 * @brief Invalidate a page in the TLB
 * @param addr The address of the page to invalidate
 */
static void invlpg(uint32_t addr)
{
    __asm__ volatile ("invlpg (%0)" : : "r"(addr) : "memory");
}

/**
 * @brief Get the current values of CPU registers
 * @param eax Pointer to store EAX value
 * @param ebx Pointer to store EBX value
 * @param ecx Pointer to store ECX value
 * @param edx Pointer to store EDX value
 * @param esi Pointer to store ESI value
 * @param edi Pointer to store EDI value
 * @param ebp Pointer to store EBP value
 * @param esp Pointer to store ESP value
 * @param eip Pointer to store EIP value
 */
static void arch_get_registers(uint32_t* eax, uint32_t* ebx, uint32_t* ecx,
                                      uint32_t* edx, uint32_t* esi, uint32_t* edi,
                                      uint32_t* ebp, uint32_t* esp, uint32_t* eip)
{
    //save reg to prevent clobbing
    __asm__ volatile ("movl %%eax, %0" : "=m"(*eax));
    __asm__ volatile ("movl %%ebx, %0" : "=m"(*ebx));
    __asm__ volatile ("movl %%ecx, %0" : "=m"(*ecx));
    __asm__ volatile ("movl %%edx, %0" : "=m"(*edx));
    __asm__ volatile ("movl %%esi, %0" : "=m"(*esi));
    __asm__ volatile ("movl %%edi, %0" : "=m"(*edi));
    __asm__ volatile ("movl %%ebp, %0" : "=m"(*ebp));
    __asm__ volatile ("movl %%esp, %0" : "=m"(*esp));

    //intr ptr
    __asm__ volatile (
        "call 1f\n"
        "1: popl %%eax\n"
        "movl %%eax, %0"
        : "=m"(*eip)
        :
        : "eax"
    );
}

#ifdef __cplusplus
}
#endif

#endif
