#include "ref.h"

void ref_init(Ref* ref, ref_destroy_fn destroy)
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

    atomic_fetch_add_explicit(
            &ref->refs,
            1,
            memory_order_relaxed
    );

    return obj;
}

void ref_release(void* obj)
{
    if (!obj)
    {
        return;
    }

    Ref* ref = obj;

    uint32_t old = atomic_fetch_sub_explicit(
            &ref->refs,
            1,
            memory_order_acq_rel
    );

    if (old == 0)
    {
        kernel_panic("ref_release: reference count underflow");
    }

    if (old == 1)
    {
        if (ref->destroy)
        {
            ref->destroy(obj);
        }
    }
}