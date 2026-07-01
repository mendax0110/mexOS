#ifndef KERNEL_ASSERT_H
#define KERNEL_ASSERT_H

#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Method to trigger kernel panic
 * @param msg The message to display
 */
NORETURN void kernel_panic(const char* msg);

#ifdef __cplusplus
}
#endif

/**
 * @brief Assert macro, which triggers kernel panic
 * @param cond The condition to check
 */
#define ASSERT(cond)                                \
    do                                              \
    {                                               \
        if (!(cond))                                \
        {                                           \
            kernel_panic("Assert failed: " #cond);  \
        }                                           \
    }                                               \
    while (0)

#endif // KERNEL_ASSERT_H
