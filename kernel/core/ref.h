#ifndef KERNEL_REFERENCE_H
#define KERNEL_REFERENCE_H

#include "include/atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Ref Ref;
typedef void (*ref_destroy_fn)(void*);

/**
 * @brief Struct representing a reference \struct Ref
 */
struct Ref
{
    atomic_uint16_t refs;
    ref_destroy_fn destroy;
};

/**
 * @brief Initialize a reference
 * @param ref The reference to initialize
 * @param destroy The destroy function to call when the reference count reaches zero
 */
void ref_init(Ref* ref, ref_destroy_fn destroy);

/**
 * @brief Retains a reference
 * @param obj The reference to retain
 * @return The retained reference
 */
void* ref_retain(void* obj);

/**
 * @brief Releases a reference
 * @param obj The reference to release
 */
void ref_release(void* obj);

/**
 * @brief Helper to clean up a reference when going out of scope
 * @param ptr The pointer to clean up
 */
static inline void ref_cleanup(void* ptr)
{
    void** obj = ptr;

    if (*obj)
    {
        ref_release(*obj);
    }
}

#if defined(__GNUC__) || defined(__clang__)
    /**
     * @brief Macro to automatically clean up a reference when going out of scope
     */
    #define AUTO_REF \
        __attribute__((cleanup(ref_cleanup)))
#else
    #define AUTO_REF
#endif

/**
 * @brief Macro to retain a given reference
 * @param p The reference to retain
 */
#define RETAIN(p) \
    (__typeof__(p))ref_retain(p)

#ifdef __cplusplus
}
#endif

#endif // KERNEL_REFERENCE_H
