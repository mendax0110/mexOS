#ifndef KERNEL_REFERENCE_H
#define KERNEL_REFERENCE_H

#include "include/atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Ref Ref;
typedef void (*ref_destroy_fn)(void*);

struct Ref
{
    atomic_uint16_t refs;
    ref_destroy_fn destroy;
};

void ref_init(Ref* ref, ref_destroy_fn destroy);
void* ref_retain(void* obj);
void ref_release(void* obj);

static inline void ref_cleanup(void* ptr)
{
    void** obj = ptr;

    if (*obj)
    {
        ref_release(*obj);
    }
}

#if defined(__GNUC__) || defined(__clang__)
    #define AUTO_REF \
        __attribute__((cleanup(ref_cleanup)))
#else
    #define AUTO_REF
#endif

#define RETAIN(p) \
    (__typeof__(p))ref_retain(p)

#ifdef __cplusplus
}
#endif

#endif // KERNEL_REFERENCE_H
