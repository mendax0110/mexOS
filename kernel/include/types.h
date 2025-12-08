#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

/**
 * @brief Standard type definitions
 */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;
typedef uint32_t           size_t;
typedef int32_t            ssize_t;
typedef int32_t            pid_t;
typedef uint32_t           tid_t;
typedef uint64_t           uintptr_t;

/**
 * @brief NULL pointer definition
 */
#define NULL ((void*)0)

/**
 * @brief Boolean type definition
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#ifndef __bool_true_false_are_defined
typedef uint8_t bool;
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
#endif
#endif

/**
 * @brief Attribute macros for structure packing and alignment
 */
#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))

#endif
