#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

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
typedef double             float64_t;
typedef float              float32_t;

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
#if defined(__GNUC__) || defined(__clang__) || (__STDC_VERSION__ >= 199901L)
    #define bool  _Bool
    #define true  1
    #define false 0
#else
    typedef uint8_t bool;
    #define true 1
    #define false 0
#endif

/**
 * @brief Limit flag for uint32_t to indicate an invalid value
 */
#define UINT32_INVALID 0xFFFFFFFF

/**
 * @brief Limit flag for uint32_t to indicate an invalid unsigned value
 */
#define UINT32_INVALID_UNSIGNED 0xFFFFFFFFU

/**
 * @brief Maximum value for uint8_t
 */
#define UINT8_MAX 0xFF

/**
 * @brief Maximum value for uint16_t
 */
#define UINT16_MAX 0xFFFF

/**
 * @brief Maximum value for uint32_t
 */
#define UINT32_MAX 0xFFFFFFFFU

/**
 * @brief Maximum value for uint64_t
 */
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL

/**
 * @brief Maximum value for int8_t
 */
#define INT8_MAX 0x7F

/**
 * @brief Maximum value for int16_t
 */
#define INT16_MAX 0x7FFF

/**
 * @brief Maximum value for int32_t
 */
#define INT32_MAX 0x7FFFFFFF

/**
 * @brief Maximum value for int64_t
 */
#define INT64_MAX 0x7FFFFFFFFFFFFFFFLL

#endif
