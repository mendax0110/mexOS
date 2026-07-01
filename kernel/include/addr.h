#ifndef KERNEL_ADDR_H
#define KERNEL_ADDR_H

#include "assert.h"
#include "types.h"

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#endif

/**
 * @brief Makes ptr from u32
 * @param x The u32 value
 */
#define PTR_FROM_U32(x)             \
({                                  \
    uint32_t _v = (uint32_t)(x);    \
    (void*)(uintptr_t)_v;           \
})

/**
 * @brief Makes ptr to u32
 * @param ptr The pointer
 */
#define PTR_TO_U32(ptr)                         \
({                                              \
    uintptr_t _v = (uintptr_t)(ptr);            \
    ASSERT(_v <= (uintptr_t)LIMIT_UNSIGNED);    \
    (uint32_t)_v;                               \
})

/**
 * @brief Gets const char from u32
 * @param x The u32 value
 */
#define CONST_CHAR_FROM_U32(x)      \
({                                  \
    uint32_t _v = (uint32_t)(x);    \
    (const char*)(uintptr_t)_v;     \
})

/**
 * @brief Gets char from u32
 * @param x The u32 value
 */
#define CHAR_FROM_U32(x)            \
({                                  \
    uint32_t _v = (uint32_t)(x);    \
    (char*)(uintptr_t)_v;           \
})

/**
 * @brief Get ptr from typed u32
 * @param type The type of the pointer
 * @param x The u32 value
 */
#define PTR_FROM_U32_TYPED(type, x)     \
({                                      \
    uint32_t _v = (uint32_t)(x);        \
    (type*)(uintptr_t)_v;               \
})

/**
 * @brief Get ptr from typed u32 in strict mode (checks alignment)
 * @param type The type of the pointer
 * @param x The u32 value
 */
#define PTR_FROM_U32_TYPED_STRICT(type, x)      \
({                                              \
    uint32_t _v = (uint32_t)(x);                \
    ASSERT(_v % __alignof__(type) == 0);        \
    (type*)(uintptr_t)_v;                       \
})

/**
 * @brief Casts a value to a pointer of a given type
 * @param type The type of the pointer
 * @param value The value to cast
 */
#define PTR_CAST(type, value)                   \
({                                              \
    uintptr_t _v = (uintptr_t)(value);          \
    ASSERT(_v <= (uintptr_t)LIMIT_UNSIGNED);    \
    (type)_v;                                   \
})

/**
 * @brief Makes funcptr to u32
 * @param fptr the function poitner
 */
#define FUNC_PTR_TO_U32(fptr)                   \
({                                              \
    uintptr_t _v = (uintptr_t)(fptr);           \
    ASSERT(_v <= (uintptr_t)LIMIT_UNSIGNED);    \
    (uint32_t)_v;                               \
})

/**
 * @brief Makes void ptr from u32
 * @param x The u32 value
 */
#define VOID_PTR_FROM_U32(x)        \
({                                  \
    uint32_t _v = (uint32_t)(x);    \
    (void*)(uintptr_t)_v;           \
})

/**
 * @brief Makes void ptr to u32
 * @param ptr The u32 value
 */
#define VOID_PTR_TO_U32(ptr)                    \
({                                              \
    uintptr_t _v = (uintptr_t)(ptr);            \
    ASSERT(_v <= (uintptr_t)LIMIT_UNSIGNED);    \
    (uint32_t)_v;                               \
})

/**
 * @brief Helper for pointer arithmetic operations
 * @param type The type
 * @param ptr The pointer
 * @param offset The offset
 */
#define PTR_ARITH(type, ptr, offset)                \
({                                                  \
    uintptr_t _base = (uintptr_t)(ptr);             \
    uintptr_t _offset = (uintptr_t)(offset);        \
    ASSERT(_base <= (uintptr_t)LIMIT_UNSIGNED);     \
    ASSERT(_offset <= (uintptr_t)LIMIT_UNSIGNED);   \
    (type*)(_base + _offset);                       \
})

/**
 * @brief Helper to create a refernce
 * @param type The type
 * @param name The name
 * @param value The value of the ref
 */
#define CREATE_REFERENCE(type, name, value) \
    type& name = *(type*)(uintptr_t)(value)

/**
 * @brief Helper to create a const reference
 * @param type The type
 * @param name The name
 * @param value The value of the const ref
 */
#define CREATE_CONST_REFERENCE(type, name, value) \
    const type& name = *(const type*)(uintptr_t)(value)

/**
 * @brief Helper to create a pointer
 * @param type The type
 * @param name The name
 * @param value The value of the pointer
 */
#define CREATE_POINTER(type, name, value) \
    type* name = (type*)(uintptr_t)(value)

/**
 * @brief Helper to create a const pointer
 * @param type The type
 * @param name The name
 * @param value The value of the const pointer
 */
#define CREATE_CONST_POINTER(type, name, value) \
    const type* name = (const type*)(uintptr_t)(value)

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // KERNEL_ADDR_H
