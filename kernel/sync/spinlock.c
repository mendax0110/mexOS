#include "spinlock.h"
#include "../arch/i686/arch.h"

void spinlock_init(spinlock_t* lock)
{
    if (!lock) return;
    lock->locked = 0;
}

uint32_t spinlock_acquire(spinlock_t* lock)
{
    const uint32_t flags = irq_save();

    while (__sync_lock_test_and_set(&lock->locked, 1))
    {
        // Spin, re-enable interrupts shortly so ISR can still fire
        // avoiding deadlocks if the lock holder needs IRQ to procced
        asm volatile("pause");
    }

    return flags;
}

void spinlock_release(spinlock_t* lock, uint32_t flags)
{
    __sync_lock_release(&lock->locked);
    irq_restore(flags);
}

bool spinlock_is_locked(const spinlock_t* lock)
{
    if (!lock) return false;
    return lock->locked != 0;
}