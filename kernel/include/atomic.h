#ifndef KERNEL_ATOMIC_H
#define KERNEL_ATOMIC_H

#include "../../shared/types.h"

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
 * @brief Init atomic, store
 * @param obj The ptr
 * @param value The value to store
 */
static inline void atomic_init(atomic_uint16_t* obj, const uint32_t value)
{
    __atomic_store_n(obj, value, __ATOMIC_SEQ_CST);
}

/**
 * @brief Fetch and add explicit
 * @param obj The ptr
 * @param value The value to add
 * @param memory_order The memory order
 */
static inline void atomic_fetch_add_explicit(atomic_uint16_t* obj, const uint32_t value, const memory_order_t memory_order)
{
    __atomic_fetch_add(obj, value, memory_order);
}

/**
 * @brief Fetch and sub explicit
 * @param obj The ptr
 * @param value The value to subtract
 * @param memory_order The memory order
 * @return The value before the subtraction
 */
static inline uint32_t atomic_fetch_sub_explicit(atomic_uint16_t* obj, const uint32_t value, const memory_order_t memory_order)
{
    return __atomic_fetch_sub(obj, value, memory_order);
}

#endif // KERNEL_ATOMIC_H
