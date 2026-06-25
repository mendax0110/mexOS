#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Spinlock structure for mutual exclusion \struct spinlock
 */
typedef struct
{
    volatile uint32_t locked;
} spinlock_t;

/**
 * @brief Initialize a spinlock
 */
#define SPINLOCK_INIT { 0 }

#define SPINLOCK_LOCKED 1
#define SPINLOCK_UNLOCKED 0

/**
 * @brief Initialize a spinlock
 * @param lock Pointer to the spinlock to initialize
 */
void spinlock_init(spinlock_t* lock);

/**
 * @brief Acquire a spinlock, disabling interrupts
 * @param lock Pointer to the spinlock to acquire
 * @return saved interrupt flags (pass to spinlock_release)
 */
uint32_t spinlock_acquire(spinlock_t* lock);

/**
 * @brief Release a spinlock, restoring interrupts
 * @param lock Pointer to the spinlock to release
 * @param flags The saved interrupt flags from spinlock_acquire
 */
void spinlock_release(spinlock_t* lock, uint32_t flags);


/**
 * @brief Acquire a spinlock and set its locked state, disabling interrupts
 * @param lock Pointer to the spinlock to acquire and set
 * @param flags The flags to set (usually 1 for locked)
 */
bool spinlock_sync_and_set(spinlock_t* lock, uint32_t flags);

/**
 * @brief Check if a spinlock is currently locked
 * @param lock Pointer to the spinlock to check
 * @return true if the lock is held, false otherwise
 */
bool spinlock_is_locked(const spinlock_t* lock);

#ifdef __cplusplus
}
#endif

#endif //KERNEL_SPINLOCK_H