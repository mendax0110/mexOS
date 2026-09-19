#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include "../arch/i686/arch.h"
#include "../ui/console.h"
#include "../mm/pmm.h"

/**
 * @brief A structure to map EFLAGS bits to their string representations \struct eflag_print_map
 */
struct eflag_print_map
{
    uint32_t bit;
    const char* name;
};

/**
 * @brief A structure to map CR0 bits to their string representations \struct cr0_print_map
 */
struct cr0_print_map
{
    uint8_t bit;
    const char* name;
};

/**
 * @brief Handle a kernel panic by displaying the message and halting the system
 * @param msg The panic message to display
 */
NORETURN void kernel_panic(const char* msg);


#endif //KERNEL_PANIC_H
