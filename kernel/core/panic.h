#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include "../arch/i686/arch.h"
#include "../ui/console.h"
#include "../mm/pmm.h"

/**
 * @brief Handle a kernel panic by displaying the message and halting the system
 * @param msg The panic message to display
 */
void kernel_panic(const char* msg);


#endif //KERNEL_PANIC_H
