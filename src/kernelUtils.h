#pragma once
#include "mexKernel.h"

/**
 * Convert an integer to a string representation.
 * @param num -> The integer to convert.
 * @return A pointer to a static string containing the integer representation.
 */
const char* int_to_str(uint32_t num);

/**
 * Convert a hexadecimal number to a string representation.
 * @param num -> The hexadecimal number to convert.
 * @return A pointer to a static string containing the hexadecimal representation.
 */
const char* hex_to_str(uint32_t num);

/**
 * @brief Strlen function that calculates the length of a string.
 * @param str The string whose length is to be calculated.
 * @return The length of the string, excluding the null terminator.
 */
uint32_t strlen(const char* str);

/**
 * @brief Compares two strings for equality.
 * @param str1 The first string to compare.
 * @param str2 The second string to compare.
 * @return 0 if the strings are equal, a positive value if str1 is greater than str2, or a negative value if str1 is less than str2.
 */
int strcmp(const char* str1, const char* str2);

/**
 * @brief Copies a string from source to destination.
 * @param dest The destination buffer where the string will be copied.
 * @param src The source string to copy from.
 */
void strcpy(char* dest, const char* src);

/**
 * @brief Memory allocation function that allocates a block of memory of the specified size.
 * @param size The size of the memory block to allocate in bytes.
 * @return A pointer to the allocated memory block, or nullptr if the allocation fails.
 */
void *kmalloc(uint32_t size);

/**
 * @brief Prints the shell prompt.
 * @param prompt The prompt string to be printed.
 */
void *memset(void* dest, int value, uint32_t size);