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
 * @brief Atomic load with explicit memory order
 * @param obj The pointer to the atomic variable
 * @param order The memory order for the load operation
 * @return The loaded value from the atomic variable
 */
static inline uint16_t atomic_load_explicit(const atomic_uint16_t* obj, const memory_order_t order)
{
    return __atomic_load_n(obj, order);
}

/**
 * @brief Atomic store with explicit memory order
 * @param obj The pointer to the atomic variable
 * @param value The value to store
 * @param order The memory order for the store operation
 */
static inline void atomic_store_explicit(atomic_uint16_t* obj, const uint16_t value, const memory_order_t order)
{
    __atomic_store_n(obj, value, order);
}

/**
 * @brief Init atomic, store
 * @param obj The ptr
 * @param value The value to store
 */
static inline void atomic_init(atomic_uint16_t* obj, const uint16_t value)
{
    atomic_store_explicit(obj, value, memory_order_seq_cst);
}

/**
 * @brief Fetch and add explicit
 * @param obj The ptr
 * @param value The value to add
 * @param memory_order The memory order
 * @return The value before the addition
 */
static inline uint16_t atomic_fetch_add_explicit(atomic_uint16_t* obj, const uint16_t value, const memory_order_t memory_order)
{
    return __atomic_fetch_add(obj, value, memory_order);
}

/**
 * @brief Fetch and sub explicit
 * @param obj The ptr
 * @param value The value to subtract
 * @param memory_order The memory order
 * @return The value before the subtraction
 */
static inline uint16_t atomic_fetch_sub_explicit(atomic_uint16_t* obj, const uint16_t value, const memory_order_t memory_order)
{
    return __atomic_fetch_sub(obj, value, memory_order);
}

/**
 * @brief Compare and exchange strong explicit
 * @param obj The ptr
 * @param expected The expected value
 * @param desired The desired value
 * @param success The memory order for success
 * @param failure The memory order for failure
 * @return true if the exchange was successful, false otherwise
 */
static inline bool atomic_compare_exchange_strong_explicit(atomic_uint16_t* obj, uint16_t* expected,
                                                            const uint16_t desired, const memory_order_t success,
                                                            const memory_order_t failure)
{
    return __atomic_compare_exchange_n(obj, expected, desired, false, success, failure);
}

/**
 * @brief Compare and exchange weak explicit
 * @param obj The ptr
 * @param expected The expected value
 * @param desired The desired value
 * @param success The memory order for success
 * @param failure The memory order for failure
 * @return true if the exchange was successful, false otherwise
 */
static inline bool atomic_compare_exchange_weak_explicit(atomic_uint16_t* obj, uint16_t* expected,
                                                          const uint16_t desired, const memory_order_t success,
                                                          const memory_order_t failure)
{
    return __atomic_compare_exchange_n(obj, expected, desired, true, success, failure);
}

#endif // KERNEL_ATOMIC_H
