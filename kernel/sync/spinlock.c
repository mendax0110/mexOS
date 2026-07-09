#include "spinlock.h"

#include "asm.h"
#include "arch/i686/arch.h"

void spinlock_init(spinlock_t* lock)
{
    if (!lock) { return; }
    lock->locked = 0;
}

bool spinlock_sync_and_set(spinlock_t* lock, const uint32_t flags)
{
    return __sync_lock_test_and_set(&lock->locked, flags) == 0;
}

uint32_t spinlock_acquire(spinlock_t* lock)
{
    const uint32_t flags = irq_save();

    while (!spinlock_sync_and_set(lock, SPINLOCK_LOCKED))
    {
        // Spin, re-enable interrupts shortly so ISR can still fire
        // avoiding deadlocks if the lock holder needs IRQ to procced
        ASM_V("pause");
    }

    return flags;
}

void spinlock_release(spinlock_t* lock, const uint32_t flags)
{
    __sync_lock_release(&lock->locked);
    irq_restore(flags);
}

bool spinlock_is_locked(const spinlock_t* lock)
{
    if (!lock) { return false; }
    return lock->locked != 0;
}