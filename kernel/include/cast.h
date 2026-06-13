#ifndef KERNEL_CAST_H
#define KERNEL_CAST_H

#include "types.h"
#include "kernel.h"

#define ASSERT(cond) \
    do \
    { \
        if (!(cond)) \
        { \
            kernel_panic("Assert failed: " #cond); \
        }\
    } \
    while (0) \

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#endif

//#define PTR_FROM_U32(x) ((void*)(uintptr_t)(x))
#define PTR_FROM_U32(x) ({ uint32_t _v = (uint32_t)(x); (void*)(uintptr_t)_v; })
//#define PTR_TO_U32(ptr) ((uint32_t)(uintptr_t)(ptr))
#define PTR_TO_U32(ptr) ({ uintptr_t _v = (uintptr_t)(ptr);  ASSERT(_v <= (uintptr_t)0xFFFFFFFFU); (uint32_t)_v; })
//#define CONST_CHAR_FROM_U32(x) ((const char*)(uintptr_t)(x))
#define CONST_CHAR_FROM_U32(x) ({ uint32_t _v = (uint32_t)(x); (const char*)(uintptr_t)_v; })
//#define CHAR_FROM_U32(x) ((char*)(uintptr_t)(x))
#define CHAR_FROM_U32(x) ({ uint32_t _v = (uint32_t)(x); (char*)(uintptr_t)_v; })
//#define PTR_FROM_U32_TYPED(type,x) ((type*)(uintptr_t)(x))
#define PTR_FROM_U32_TYPED(type,x) ({ \
    uint32_t _v = (uint32_t)(x); \
    (type*)(uintptr_t)_v; \
})
#define PTR_FROM_U32_TYPED_STRICT(type, x) ({ \
    uint32_t _v = (uint32_t)(x); \
    ASSERT(_v % __alignof__(type) == 0); \
    (type*)(uintptr_t)_v; \
})
//#define PTR_CAST(type,value) ((type)(uintptr_t)(value))
#define PTR_CAST(type,value) ({ \
    uintptr_t _v = (uintptr_t)(value); \
    ASSERT(_v <= (uintptr_t)0xFFFFFFFFU); \
    (type)_v; \
})
//#define FUNC_PTR_TO_U32(fptr) ((uint32_t)(uintptr_t)(fptr))
#define FUNC_PTR_TO_U32(fptr)  ({ uintptr_t _v = (uintptr_t)(fptr); ASSERT(_v <= (uintptr_t)0xFFFFFFFFU); (uint32_t)_v; })
#define BIT_FLAG(val, mask) ((uint32_t)((val) & (mask)))
//#define BIT_FLAG_SET(val, mask) ((uint32_t)((val) | (mask)))
#define BIT_FLAG_SET(val, mask) ({ ASSERT((mask) != 0); (uint32_t)((val) | (mask)); })
//#define BIT_FLAG_CLEAR(val, mask) ((uint32_t)((val) & ~(mask)))
#define BIT_FLAG_CLEAR(val, mask) ({ ASSERT((mask) != 0); (uint32_t)((val) & ~(mask)); })

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // KERNEL_CAST_H
