#ifndef KERNEL_ASSERT_H
#define KERNEL_ASSERT_H

#include "../shared/compiler.h"
#include "../diag/panic.h"

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

#define STATIC_ASSERT(cond, msg) \
    _Static_assert(cond, msg)


#endif // KERNEL_ASSERT_H
