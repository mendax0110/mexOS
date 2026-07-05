#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

#include "compiler.h"

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
typedef uint32_t           uintptr_t;
typedef __builtin_va_list  va_list;

/**
 * @brief NULL pointer definition
 */
#if __STDC_VERSION__ >= 201112L
    #define NULL ((void*)0)
#elif __STDC_VERSION__ >= 202311L
    #define NULL nullptr
#elif defined(__GNUC__)
    #define NULL ((void*)0)
#else
    #define NULL ((void*)0)
#endif

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
 * @brief Limit flag for uint32_t to indicate an invalid value
 */
#define LIMIT 0xFFFFFFFF

/**
 * @brief Limit flag for uint32_t to indicate an invalid unsigned value
 */
#define LIMIT_UNSIGNED 0xFFFFFFFFU

#endif
