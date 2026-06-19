#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set a block of memory to a specific value
 * @param dest Pointer to the destination memory
 * @param val Value to set
 * @param len Number of bytes to set
 * @return Pointer to the destination memory
 */
void* memset(void* dest, int val, size_t len);

/**
 * @brief Copy a block of memory from source to destination
 * @param dest Pointer to the destination memory
 * @param src Pointer to the source memory
 * @param len Number of bytes to copy
 * @return Pointer to the destination memory
 */
void* memcpy(void* dest, const void* src, size_t len);

/**
 * @brief Compare two blocks of memory
 * @param s1 Pointer to the first memory block
 * @param s2 Pointer to the second memory block
 * @param len Number of bytes to compare
 * @return An integer less than, equal to, or greater than zero if s1 is found,
 *         respectively, to be less than, to match, or be greater than s2
 */
int memcmp(const void* s1, const void* s2, size_t len);

/**
 * @brief Calculate the length of a null-terminated string
 * @param str Pointer to the string
 * @return The length of the string
 */
size_t strlen(const char* str);

/**
 * @brief Compare two null-terminated strings
 * @param s1 Pointer to the first string
 * @param s2 Pointer to the second string
 * @return An integer less than, equal to, or greater than zero if s1 is found,
 *         respectively, to be less than, to match, or be greater than s2
 */
int strcmp(const char* s1, const char* s2);

/**
 * @brief Compare two null-terminated strings up to a specified number of characters
 * @param s1 Pointer to the first string
 * @param s2 Pointer to the second string
 * @param n Maximum number of characters to compare
 * @return An integer less than, equal to, or greater than zero if s1 is found,
 *         respectively, to be less than, to match, or be greater than s2
 */
int strncmp(const char* s1, const char* s2, size_t n);

/**
 * @brief Copy a null-terminated string from source to destination
 * @param dest Pointer to the destination string
 * @param src Pointer to the source string
 * @return Pointer to the destination string
 */
char* strcpy(char* dest, const char* src);

/**
 * @brief Copy a string from source to destination up to a specified number of characters
 * @param dest Pointer to the destination string
 * @param src Pointer to the source string
 * @param n Maximum number of characters to copy
 * @return Pointer to the destination string
 */
char* strncpy(char* dest, const char* src, size_t n);

/**
 * @brief Concatenate two strings
 * @param dest Pointer to the destination string
 * @param src Pointer to the source string
 * @return Pointer to the destination string
 */
char* strcat(char* dest, const char* src);

/**
 * @brief Concatenate two strings up to a specified number of characters
 * @param dest Pointer to the destination string
 * @param src Pointer to the source string
 * @param n Maximum number of characters to concatenate
 * @return Pointer to the destination string
 */
char* strncat(char* dest, const char* src, size_t n);

/**
 * @brief Convert an integer to a string with padding
 * @param value The integer value to convert
 * @param str Pointer to the destination string buffer
 * @param width Minimum width of the resulting string (padded with leading zeros if necessary)
 */
void int_to_str_pad(int value, char* str, int width);

/**
 * @brief Convert an unsigned integer to a hexadecimal string with padding
 * @param value The unsigned integer value to convert
 * @param str Pointer to the destination string buffer
 * @param width Minimum width of the resulting string (padded with leading zeros if necessary)
 */
void int_to_hex_pad(uint32_t value, char* str, int width);

/**
 * @brief Convert an integer to a string
 * @param value The integer value to convert
 * @param str Pointer to the destination string buffer
 * @param base Number base (2-36)
 * @return Pointer to the destination string
 */
char* itoa(int value, char* str, int base);

#ifdef __cplusplus
}
#endif

#endif
