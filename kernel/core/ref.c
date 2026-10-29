#include "core/ref.h"
#include "include/assert.h"

void ref_init(Ref* ref, const ref_destroy_fn destroy)
{
    atomic_init(&ref->refs, 1);
    ref->destroy = destroy;
}

void* ref_retain(void* obj)
{
    if (!obj)
    {
        return NULL;
    }

    Ref* ref = obj;

    const uint16_t old = atomic_fetch_add_explicit(
            &ref->refs,
            1,
            memory_order_relaxed
    );

    ASSERT_FMT(old != UINT16_MAX, "ref_retain: reference count overflow (old=%u)", old);

    return obj;
}

void ref_release(void* obj)
{
    if (!obj)
    {
        return;
    }

    Ref* ref = obj;

    const uint16_t old = atomic_fetch_sub_explicit(
            &ref->refs,
            1,
            memory_order_acq_rel
    );

    ASSERT_FMT(old > 0, "ref_release: reference count underflow (old=%u)", old);

    if (old == 1 && ref->destroy)
    {
        ref->destroy(obj);
    }
}
