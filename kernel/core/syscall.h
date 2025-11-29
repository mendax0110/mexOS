#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include "../include/types.h"
#include "../arch/i686/idt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System call numbers
 */
#define SYS_EXIT     0
#define SYS_WRITE    1
#define SYS_READ     2
#define SYS_YIELD    3
#define SYS_GETPID   4
#define SYS_FORK     5
#define SYS_WAIT     6
#define SYS_EXEC     7
#define SYS_OPEN     8
#define SYS_CLOSE    9
#define SYS_SEND     10
#define SYS_RECV     11
#define SYS_PORT_CREATE 12
#define SYS_PORT_DESTROY 13

/**
 * @brief Initialize the syscall handler
 */
void syscall_init(void);

/**
 * @brief Syscall handler function
 * @param regs Pointer to the registers structure
 * @return The result of the syscall
 */
int syscall_handler(struct registers* regs);

#ifdef __cplusplus
}
#endif

#endif
