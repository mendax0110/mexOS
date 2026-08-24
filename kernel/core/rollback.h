#ifndef KERNEL_ROLLBACK_H
#define KERNEL_ROLLBACK_H

#include "include/assert.h"
#include "include/source_location.h"
#include "../../shared/types.h"
#include "lib/log.h"

#if defined(__clang__)
    /**
     * @brief Helper macro to create "lambdas" in c
     * @param ret The return type
     * @param args The arguments
     * @param body The body
     */
    #define LAMBDA(ret, args, body) (^ret args body)
    typedef void (^rollback_fn_t)(void);
    #define ROLLBACK_WRAP(fn) ((rollback_fn_t)(fn))
#elif defined(__GNUC__)
    /**
     * @brief Helper macro to create "lambdas" in c
     * @param ret The return type
     * @param args The arguments
     * @param body The body
     */
    #define LAMBDA(ret, args, body)         \
            __extension__                   \
            ({                              \
                auto ret _fn_ args body     \
                _fn_;                       \
            })

    /**
     * @brief Typedef for rollback function pointer \typedef rollback_fn_t
     */
    typedef void (*rollback_fn_t)(void);

    /**
     * @brief Wrap a function pointer as a rollback function
     * @param fn The function to wrap
     */
    #define ROLLBACK_WRAP(fn) ((rollback_fn_t)(fn))
#endif

/**
 * @brief Struct to represent fault context \struct fault_ctx_t
 */
typedef struct fault_ctx
{
    const char* name;
    rollback_fn_t rollback;
    const char* file;
    int line;
    bool rolled_back;
    struct fault_ctx* prev;
} fault_ctx_t;

/**
 * @brief Global var for fault context
 */
extern fault_ctx_t* g_fault_ctx;

/**
 * @brief Push a fault
 * @param ctx The context
 */
void fault_push(fault_ctx_t* ctx);

/**
 * @brief Pop a fault
 */
void fault_pop(void);

/**
 * @brief Rollback current state
 */
void rollback_current(void);

/**
 * @brief Helper macro to try context
 * @param name_ The name of the context
 * @param rollback_ The rollback function
 */
#define TRY_CTX(name_, rollback_)                                       \
    for (fault_ctx_t _ctx = {                                           \
            (name_),                                                    \
            ROLLBACK_WRAP(rollback_),                                   \
            __FILENAME__,                                               \
            __LINE__,                                                   \
            false,                                                      \
            NULL                                                        \
        },                                                              \
        *_once = (fault_push(&_ctx), (fault_ctx_t*)1);                  \
        _once;                                                          \
        fault_pop(), _once = NULL)

/**
 * @brief Helper macro to rollback current context
 */
#define ROLLBACK() rollback_current()

/**
 * @brief Helper macro to throw a fault
 */
#define THROW()                                                         \
    do                                                                  \
    {                                                                   \
        ROLLBACK();                                                     \
        if (g_fault_ctx)                                                \
        {                                                               \
            PANIC_FMT("fault thrown in %s (%s:%d)",                     \
                g_fault_ctx->name,                                      \
                g_fault_ctx->file,                                      \
                g_fault_ctx->line);                                     \
        }                                                               \
        else                                                            \
        {                                                               \
            PANIC_FMT("fault thrown outside of any context");           \
        }                                                               \
    }                                                                   \
    while (0)

#endif // KERNEL_ROLLBACK_H
