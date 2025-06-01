#pragma once

#include "mexKernel.h"

/**
 * @brief Prints a string to the console.
 * @param str The string to be printed.
 */
void print(const char* str);

/**
 * @brief Prints the shell prompt.
 */
void shell();

/**
 * @brief Makes a system call to the kernel.
 * @param num The system call number.
 * @param arg1 The first argument for the system call.
 * @param arg2 The second argument for the system call.
 * @param arg3 The third argument for the system call.
 * @return An integer representing the return value of the system call.
 */
static inline int syscall(int num, int arg1 = 0, int arg2 = 0, int arg3 = 0)
{
    int ret;
    asm volatile (
            "int $0x80"
            : "=a" (ret)
            : "a" (num), "b" (arg1), "c" (arg2), "d" (arg3)
            );
    return ret;
}