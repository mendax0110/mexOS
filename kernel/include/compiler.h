#ifndef KERNEL_COMPILER_H
#define KERNEL_COMPILER_H

#include "include/types.h"

/**
 * @brief Feature-detection helpers
 */
#ifndef __STDC_VERSION__
    #define KCOMPILER_STDC_VERSION 0L
#else
    #define KCOMPILER_STDC_VERSION __STDC_VERSION__
#endif

/**
 * @brief Internal helper used by other macros for evaluation
 */
#if defined(__has_c_attribute)
    #define KCOMPILER_HAS_C_ATTR(attr) __has_c_attribute(attr)
#else
    #define KCOMPILER_HAS_C_ATTR(attr) 0
#endif

/**
 * @brief Internal helper used by other macros for evaluation
 */
#if defined(__has_attribute)
    #define KCOMPILER_HAS_GNU_ATTR(attr) __has_attribute(attr)
#else
    #define KCOMPILER_HAS_GNU_ATTR(attr) 0
#endif

/**
 * @brief Internal helper used by other macros for evaluation
 */
#if defined(__GNUC__) || defined(__clang__)
    #define KCOMPILER_IS_GNU_LIKE 1
#else
    #define KCOMPILER_IS_GNU_LIKE 0
#endif

/**
 * @brief PACKED and ALIGNED macro helper
 */
#if KCOMPILER_IS_GNU_LIKE
    #define PACKED __attribute__((packed))
    #define ALIGNED(x) __attribute__((aligned(x)))
#else
    #define PACKED
    #define ALIGNED(x)
#endif

/**
 * @brief Standard-attribute equivalent of PACKED/ALIGNED, for LEADING position only
 */
#if KCOMPILER_STDC_VERSION >= 202311L && KCOMPILER_HAS_C_ATTR(gnu::packed)
    #define PACKED_ATTR [[gnu::packed]]
#elif KCOMPILER_IS_GNU_LIKE
    #define PACKED_ATTR __attribute__((packed))
#else
    #define PACKED_ATTR
#endif

/**
 * @brief NORETURN macro helper
 */
#if KCOMPILER_STDC_VERSION >= 202311L && KCOMPILER_HAS_C_ATTR(noreturn)
    #define NORETURN [[noreturn]]
#elif KCOMPILER_STDC_VERSION >= 201112L
    #define NORETURN _Noreturn
#elif KCOMPILER_IS_GNU_LIKE
    #define NORETURN __attribute__((noreturn))
#else
    #define NORETURN
#endif

/**
 * @brief FALLTHROUGH macro helper
 */
#if KCOMPILER_STDC_VERSION >= 202311L && KCOMPILER_HAS_C_ATTR(fallthrough)
    #define FALLTHROUGH() [[fallthrough]]
#elif KCOMPILER_HAS_GNU_ATTR(fallthrough)
    #define FALLTHROUGH() __attribute__((fallthrough))
#else
    #define FALLTHROUGH() ((void)0) /* fallthrough */
#endif

#define KCOMPILER_STRINGIFY_(...) #__VA_ARGS__
#define KCOMPILER_STRINGIFY(...) KCOMPILER_STRINGIFY_(__VA_ARGS__)

/**
 * @brief UNUSED macro helper
 * @param x The value to ignore
 */
#if KCOMPILER_IS_GNU_LIKE
    #define UNUSED(x, ...)                                                                                          \
        do                                                                                                          \
        {                                                                                                           \
            (void)(x);                                                                                              \
            _Pragma(KCOMPILER_STRINGIFY(message "TODO(UNUSED): " #x " is not used" __VA_OPT__(" - " __VA_ARGS__)))  \
        } while (0)
#else
    #define UNUSED(x, ...) ((void)(x))
#endif

/**
 * @brief Mark a function/variable declaration itself as intentionally, unlike UNUSED(x) which silences a specific use site
 */
#if KCOMPILER_STDC_VERSION >= 202311L && KCOMPILER_HAS_C_ATTR(maybe_unused)
    #define MAYBE_UNUSED [[maybe_unused]]
#elif KCOMPILER_HAS_GNU_ATTR(unused)
    #define MAYBE_UNUSED __attribute__((unused))
#else
    #define MAYBE_UNUSED
#endif

/* ------------------------------------------------------------------------
 * Extra attributes worth having in a freestanding/kernel context
 * ------------------------------------------------------------------------ */

/**
 * @brief NODISCARD macro helper
 */
#if KCOMPILER_STDC_VERSION >= 202311L && KCOMPILER_HAS_C_ATTR(nodiscard)
    #define NODISCARD [[nodiscard]]
#elif KCOMPILER_HAS_GNU_ATTR(warn_unused_result)
    #define NODISCARD __attribute__((warn_unused_result))
#else
    #define NODISCARD
#endif

/**
 * @brief DEPRECATED macro helper
 */
#if KCOMPILER_STDC_VERSION >= 202311L && KCOMPILER_HAS_C_ATTR(deprecated)
    #define DEPRECATED [[deprecated]]
#elif KCOMPILER_HAS_GNU_ATTR(deprecated)
    #define DEPRECATED __attribute__((deprecated))
#else
    #define DEPRECATED
#endif

/**
 * @brief Force-inline regardless of optimization level (ISR/hot-path helpers).
 */
#if KCOMPILER_HAS_GNU_ATTR(always_inline)
    #define ALWAYS_INLINE inline __attribute__((always_inline))
#else
    #define ALWAYS_INLINE inline
#endif

/**
 * @brief Prevent inlining (useful for keeping a symbol callable from asm/debuggers).
 */
#if KCOMPILER_HAS_GNU_ATTR(noinline)
    #define NOINLINE __attribute__((noinline))
#else
    #define NOINLINE
#endif

/**
 * @brief Keep a symbol even if it looks unreferenced (e.g. ISR stubs referenced only from asm).
 */
#if KCOMPILER_HAS_GNU_ATTR(used)
    #define USED __attribute__((used))
#else
    #define USED
#endif

/**
 * @brief Emit a weak symbol (overridable default implementation).
 */
#if KCOMPILER_HAS_GNU_ATTR(weak)
    #define WEAK __attribute__((weak))
#else
    #define WEAK
#endif

/**
 * @brief Branch-prediction hints. Wrap the *condition*, not the whole statement.
 */
#if KCOMPILER_IS_GNU_LIKE
    #define LIKELY(cond)   (__builtin_expect(!!(cond), 1))
    #define UNLIKELY(cond) (__builtin_expect(!!(cond), 0))
#else
    #define LIKELY(cond)   (cond)
    #define UNLIKELY(cond) (cond)
#endif

/**
 * @brief Convenience macro to get the size of a given array
 * @param arr The array to get the size from
 */
#define ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Variable arguments start helper
 * @param ap The va_list to initialize
 * @param last The last named argument
 */
#define VA_START(ap, last) __builtin_va_start(ap, last)

/**
 * @brief Variable arguments helper
 * @param ap The va_list to use
 * @param type The type of the argument to retrieve
 */
#define VA_ARG(ap, type) __builtin_va_arg(ap, type)

/**
 * @brief Variable arguments stop helper
 * @param ap The va_list to stop
 */
#define VA_END(ap) __builtin_va_end(ap)

#endif // KERNEL_COMPILER_H