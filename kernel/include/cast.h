#ifndef KERNEL_CAST_H
#define KERNEL_CAST_H

#include "types.h"
#include "kernel.h"

#define ASSERT(cond)                                \
    do                                              \
    {                                               \
        if (!(cond))                                \
        {                                           \
            kernel_panic("Assert failed: " #cond);  \
        }                                           \
    }                                               \
    while (0)                                       \

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#endif

#define PTR_FROM_U32(x)             \
({                                  \
    uint32_t _v = (uint32_t)(x);    \
    (void*)(uintptr_t)_v;           \
})

#define PTR_TO_U32(ptr)                     \
({                                          \
    uintptr_t _v = (uintptr_t)(ptr);        \
    ASSERT(_v <= (uintptr_t)0xFFFFFFFFU);   \
    (uint32_t)_v;                           \
})

#define CONST_CHAR_FROM_U32(x)      \
({                                  \
    uint32_t _v = (uint32_t)(x);    \
    (const char*)(uintptr_t)_v;     \
})

#define CHAR_FROM_U32(x)            \
({                                  \
    uint32_t _v = (uint32_t)(x);    \
    (char*)(uintptr_t)_v;           \
})

#define PTR_FROM_U32_TYPED(type,x)      \
({                                      \
    uint32_t _v = (uint32_t)(x);        \
    (type*)(uintptr_t)_v;               \
})

#define PTR_FROM_U32_TYPED_STRICT(type, x)      \
({                                              \
    uint32_t _v = (uint32_t)(x);                \
    ASSERT(_v % __alignof__(type) == 0);        \
    (type*)(uintptr_t)_v;                       \
})

#define PTR_CAST(type,value)                \
({                                          \
    uintptr_t _v = (uintptr_t)(value);      \
    ASSERT(_v <= (uintptr_t)0xFFFFFFFFU);   \
    (type)_v;                               \
})

#define FUNC_PTR_TO_U32(fptr)               \
({                                          \
    uintptr_t _v = (uintptr_t)(fptr);       \
    ASSERT(_v <= (uintptr_t)0xFFFFFFFFU);   \
    (uint32_t)_v;                           \
})

#define DESCRIBE_PTR(ptr)                      \
    "0x" + itoa((int)(uintptr_t)(ptr), NULL, 16)

#define PTR_ARITH(type, ptr, offset)                \
({                                                  \
    uintptr_t _base = (uintptr_t)(ptr);             \
    uintptr_t _offset = (uintptr_t)(offset);        \
    ASSERT(_base <= (uintptr_t)0xFFFFFFFFU);        \
    ASSERT(_offset <= (uintptr_t)0xFFFFFFFFU);      \
    (type*)(_base + _offset);                       \
})

#define CREATE_REFERENCE(type, name, value) \
    type& name = *(type*)(uintptr_t)(value)

#define CREATE_CONST_REFERENCE(type, name, value) \
    const type& name = *(const type*)(uintptr_t)(value)

#define CREATE_POINTER(type, name, value) \
    type* name = (type*)(uintptr_t)(value)

#define CREATE_CONST_POINTER(type, name, value) \
    const type* name = (const type*)(uintptr_t)(value)

#define BIT_MASK(val, mask) ((uint32_t)((val) & (mask)))
#define BIT_FLAG(val, bit) (((val) & (1U << (bit))) != 0)

#define BIT(bit) (1U << (bit))
#define TEST_BIT(val, bit) (((val) & BIT(bit)) != 0)

/**
 * @brief Rollback function type for error handling
 */
#if defined(__clang__)
    #define LAMBDA(ret, args, body) (^ret args body)
    typedef void (^rollback_fn_t)(void);
    #define ROLLBACK_WRAP(fn) ((rollback_fn_t)(fn))

#elif defined(__GNUC__)
    #define LAMBDA(ret, args, body)         \
            __extension__                   \
            ({                              \
                auto ret _fn_ args body     \
                _fn_;                       \
            })
    typedef void (*rollback_fn_t)(void);
    #define ROLLBACK_WRAP(fn) ((rollback_fn_t)(fn))
#endif

/**
 * @brief Fault context structure for error handling \struct fault_ctx
 */
typedef struct fault_ctx
{
    const char* name;
    void* rollback;
    const char* file;
    int line;
    bool rolled_back;
    struct fault_ctx* prev;
} fault_ctx_t;
static fault_ctx_t* g_fault_ctx = NULL;

/**
 * @brief Push a new fault context onto the stack
 * @param ctx Pointer to the fault context to push
 */
static inline void fault_push(fault_ctx_t* ctx)
{
    ctx->prev = g_fault_ctx;
    g_fault_ctx = ctx;
}

/**
 * @brief Pop the current fault context from the stack
 */
static inline void fault_pop(void)
{
    if (g_fault_ctx)
    {
        g_fault_ctx = g_fault_ctx->prev;
    }
}

#define TRY_CTX(name_, rollback_)                                       \
    for (fault_ctx_t _ctx = {                                           \
            (name_),                                                    \
            (void*)ROLLBACK_WRAP(rollback_),                            \
            __FILE__,                                                   \
            __LINE__,                                                   \
            false,                                                      \
            NULL                                                        \
        },                                                              \
        *_once = (fault_push(&_ctx), (fault_ctx_t*)1);                  \
        _once;                                                          \
        fault_pop(), _once = NULL)

#if defined(__clang__) || defined(__GNUC__)
    #define ROLLBACK()                                              \
            do                                                      \
            {                                                       \
                if (g_fault_ctx                                     \
                    && g_fault_ctx->rollback                        \
                    && !g_fault_ctx->rolled_back)                   \
                {                                                   \
                    g_fault_ctx->rolled_back = true;                \
                    ((rollback_fn_t)g_fault_ctx->rollback)();       \
                }                                                   \
            }                                                       \
            while(0)
#endif

#define THROW()                                                         \
    do                                                                  \
    {                                                                   \
        ROLLBACK();                                                     \
        if (g_fault_ctx)                                                \
        {                                                               \
            log_error_fmt("fault thrown in %s (%s:%d)",                 \
                g_fault_ctx->name,                                      \
                g_fault_ctx->file,                                      \
                g_fault_ctx->line);                                     \
        }                                                               \
        kernel_panic("fault thrown");                                   \
    }                                                                   \
    while(0)

#define FALLTHROUGH() __attribute__((fallthrough))

#define UNUSED(x) (void)(x)

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // KERNEL_CAST_H
