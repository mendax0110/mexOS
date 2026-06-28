#ifndef KERNEL_REFERENCE_H
#define KERNEL_REFERENCE_H

#include "include/types.h"
#include "include/cast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Ref Ref;
typedef void (*ref_destroy_fn)(void *);

/**
 * @brief Refernece struct obj \Ref
 */
struct Ref
{
    atomic_uint16_t refs;
    ref_destroy_fn destroy;
};

/**
 * @brief Initialize a reference-counted object
 * @param ref The reference to init.
 * @param destroy the function to call when the reference count reaches zero.
 */
void ref_init(Ref* ref, ref_destroy_fn destroy);

/**
 * @brief Retains a reference-counted object
 * @param obj The object to retain
 * @return The retained object
 */
void* ref_retain(void* obj);

/**
 * @brief Releases a reference-counted object
 * @param obj The object to release
 */
void ref_release(void* obj);

/**
 * @brief Cleans up a reference-counted object
 * @param ptr Pointer to the object to clean up
 */
static inline void ref_cleanup(void* ptr)
{
    void **obj = ptr;

    if (*obj)
    {
        ref_release(*obj);
    }
}

#if defined(__GNUC__) || defined(__clang__)
    #define AUTO_REF \
        __attribute__((cleanup(ref_cleanup)))
#else
    #define AUTO_REF \
        __attribute__((cleanup(ref_cleanup)))
#endif

#define RETAIN(p) \
    (__typeof__(p))ref_retain(p)

#ifdef __cplusplus
};
#endif

#endif // KERNEL_REFERENCE_H