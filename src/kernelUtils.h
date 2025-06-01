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