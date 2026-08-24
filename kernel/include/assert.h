#ifndef KERNEL_ASSERT_H
#define KERNEL_ASSERT_H

#include "../shared/compiler.h"
#include "../diag/panic.h"
#include "../lib/log.h"

/**
 * @brief Panic macro helper, which logs the error and prints a custom kernel panic message
 * @param fmt The format string for the panic message
 */
#define PANIC_FMT(fmt, ...)                                             \
    do                                                                  \
    {                                                                   \
        char _panic_msg[LOG_MAX_MSG_LEN];                               \
        snprintf(_panic_msg, sizeof(_panic_msg), fmt, ##__VA_ARGS__);   \
        log_error(_panic_msg);                                          \
        kernel_panic(_panic_msg);                                       \
    }                                                                   \
    while (0)

/**
 * @brief Assert macro helper, which prints a kernel panic with the condition
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

/**
 * @brief Assert macro helper, which prints the condition and a custom defined message
 * @param cond The condidtion to check
 * @param fmt The custom message to print
 */
#define ASSERT_FMT(cond, fmt, ...)                                          \
    do                                                                      \
    {                                                                       \
        if (!(cond))                                                        \
        {                                                                   \
            PANIC_FMT("Assert failed: " #cond " (" fmt ")", ##__VA_ARGS__); \
        }                                                                   \
    }                                                                       \
    while (0)

#define STATIC_ASSERT(cond, msg) \
    _Static_assert(cond, msg)

#endif // KERNEL_ASSERT_H