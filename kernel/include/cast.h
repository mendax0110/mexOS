#ifndef KERNEL_CAST_H
#define KERNEL_CAST_H

#include "types.h"

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#endif

#define PTR_FROM_U32(x) ((void*)(uintptr_t)(x))
#define PTR_TO_U32(ptr) ((uint32_t)(uintptr_t)(ptr))
#define CONST_CHAR_FROM_U32(x) ((const char*)(uintptr_t)(x))
#define CHAR_FROM_U32(x) ((char*)(uintptr_t)(x))
#define PTR_FROM_U32_TYPED(type,x) ((type*)(uintptr_t)(x))
#define PTR_CAST(type,value) ((type)(uintptr_t)(value))
#define FUNC_PTR_TO_U32(fptr) ((uint32_t)(uintptr_t)(fptr))
#define BIT_FLAG(val, mask) ((uint32_t)((val) & (mask)))

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif // KERNEL_CAST_H
