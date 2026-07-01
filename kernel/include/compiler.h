#ifndef KERNEL_COMPILER_H
#define KERNEL_COMPILER_H

/**
 * @brief Convenience macro to mark a structure as packed, preventing padding between members
 */
#define PACKED __attribute__((packed))

/**
 * @brief Convenience macro to align a variable or structure member
 * @param x The alignment boundary
 */
#define ALIGNED(x) __attribute__((aligned(x)))

/**
 * @brief Convenience macro to mark a function as not returning
 */
#define NORETURN __attribute__((noreturn))

/**
 * @brief Convenience macro to indicate a switch case should fall through
 */
#define FALLTHROUGH() __attribute__((fallthrough))

/**
 * @brief Convenience macro to mark a variable as unused, preventing compiler warnings
 * @param x The variable to mark as unused
 */
#define UNUSED(x) (void)(x)

/**
 * @brief Convenience macro to get the size of a given array
 * @param arr The array to get the size from
 */
#define ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]))

#endif // KERNEL_COMPILER_H
