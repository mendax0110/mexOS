#pragma once

#include "dataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure representing an entry in the Interrupt Descriptor Table (IDT).
 */
struct IDTEntry
{
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

/**
 * @brief Structure representing the IDT pointer, used for loading the IDT.
 * It contains the limit (size of the IDT) and the base address of the IDT.
 */
struct IDTPtr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern struct IDTEntry idt[256];
extern struct IDTPtr idt_ptr;

/**
 * @brief Initializes the Interrupt Descriptor Table (IDT) and sets up exception handlers.
 */
void init_interrupts(void);

/**
 * @brief Sets an entry in the IDT.
 *
 * @param num The interrupt number (0-255).
 * @param base The base address of the handler function.
 * @param sel The segment selector for the code segment.
 * @param flags Flags for the IDT entry (e.g., type and privilege level).
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

/**
 * @brief Yield handler function that is called when a yield interrupt occurs.
 */
void yield_handler(void);

/**
 * @brief C++ wrapper for the yield handler function.
 * This function is called from assembly to yield control to the scheduler.
 */
void yield_handler_cpp(void);

#ifdef __cplusplus
} // extern "C"

/**
 * @brief Exception handler wrapper function.
 */
extern "C"
{
    void divide_by_zero_handler();
    void double_fault_handler();
    void general_protection_fault_handler();
    void page_fault_handler();
    void exception_handler_wrapper(uint32_t exception_number);
}
#endif