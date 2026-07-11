#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include "../../shared/types.h"
#include "arch/i686/idt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the syscall handler
 */
void syscall_init(void);

/**
 * @brief Syscall handler function
 * @param regs Pointer to the registers structure
 * @return The result of the syscall
 */
int syscall_handler(const struct registers* regs);

#ifdef __cplusplus
}
#endif

#endif
