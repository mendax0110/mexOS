#ifndef KERNEL_ASM_H
#define KERNEL_ASM_H

#include "compiler.h"

/**
 * @brief Helper macro for general asm volatile calls
 */
#define ASM_V(...) \
    __asm__ __volatile__(__VA_ARGS__)

/**
 * @brief Helper macro for asm label calls
 * @param label The label to jump to
 */
#define ASM_LABEL(label) \
    __asm__ __volatile__(#label ":")

/**
 * @brief Helper macro for asm goto calls
 * @param label The label to jump to
 */
#define ASM_GOTO(label) \
    __asm__ __volatile__("jmp " #label)

/**
 * @brief Helper macro for asm call calls
 * @param label The label to call
 */
#define ASM_CALL(label) \
    __asm__ __volatile__("call " #label)


#endif // KERNEL_ASM_H