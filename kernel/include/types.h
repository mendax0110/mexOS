#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

/**
 * @brief Standard type definitions
 */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;
typedef uint32_t           size_t;
typedef int32_t            ssize_t;
typedef int32_t            pid_t;
typedef uint32_t           tid_t;
typedef uint32_t           uintptr_t;

/**
 * @brief NULL pointer definition
 */
#define NULL ((void*)0)

/**
 * @brief Boolean type definition
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#ifndef __bool_true_false_are_defined
typedef uint8_t bool;
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
#endif
#endif

/**
 * @brief Attribute macros for structure packing and alignment
 */
#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))

/**
 * @brief Attribute macro for functions that do not return
 */
#define NORETURN __attribute__((noreturn))

/**
 * @brief Limit flag for uint32_t to indicate an invalid value
 */
#define LIMIT 0xFFFFFFFF

/**
 * @brief Limit flag for uint32_t to indicate an invalid unsigned value
 */
#define LIMIT_UNSIGNED 0xFFFFFFFFU

/**
 * @brief Atomic uint16_t type definition for atomic operations
 */
typedef uint16_t atomic_uint16_t;

/**
 * @brief Memory order enumeration for atomic operations \enum memory_order_t
 */
typedef enum
{
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order_t;

/**
 * @brief Initialize an atomic_uint16_t variable
 * @param obj Pointer to the atomic_uint8_t variable
 * @param value Initial value to set
 */
static inline void atomic_init(atomic_uint16_t* obj, uint32_t value)
{
    __atomic_store_n(obj, value, __ATOMIC_SEQ_CST);
}

/**
 * @brief Atomically fetch and add a value to an atomic_uint8_t variable
 * @param obj Pointer to the atomic_uint16_t variable
 * @param value Value to add
 * @param memory_order Memory order for the operation
 */
static inline void atomic_fetch_add_explicit(atomic_uint16_t* obj, uint32_t value, memory_order_t memory_order)
{
    __atomic_fetch_add(obj, value, memory_order);
}

/**
 * @brief Atomically fetch and subtract a value from an atomic_uint8_t variable
 * @param obj Pointer to the atomic_uint16_t variable
 * @param value Value to subtract
 * @param memory_order Memory order for the operation
 * @return The value of the atomic_uint16_t variable before the subtraction
 */
static inline uint32_t atomic_fetch_sub_explicit(atomic_uint16_t* obj, uint32_t value, memory_order_t memory_order)
{
    return __atomic_fetch_sub(obj, value, memory_order);
}

#endif
