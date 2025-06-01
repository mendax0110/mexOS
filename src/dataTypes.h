#pragma once

/**
 * @brief Own implementation of standard data types.
 */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/**
 * @brief Function pointer types for tasks in the real-time scheduler.
 */
typedef void (*TaskFunc)(void*);

/**
 * @brief Function pointer type for tasks that do not require a context.
 */
typedef void (*VoidFunc)();

/**
 * @brief Function pointer type for system calls.
 */
typedef int (*SyscallFunc)(uint32_t, uint32_t, uint32_t, uint32_t);

/**
 * @brief Function pointer type for interrupt handlers.
 */
typedef uint8_t (*InterruptHandler)(uint32_t, uint32_t, uint32_t, uint32_t);

/**
 * @brief Function pointer type for exception handlers.
 */
typedef uint8_t (*ExceptionHandler)(uint32_t, uint32_t, uint32_t, uint32_t);

/**
 * @brief Function pointer type for signal handlers.
 */
typedef uint8_t (*SignalHandler)(uint32_t, uint32_t, uint32_t, uint32_t);