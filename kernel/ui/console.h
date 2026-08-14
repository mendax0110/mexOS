#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include "../../shared/types.h"

/**
 * @brief VGA text mode constants and color definitions
 */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

#define VGA_BLACK        0
#define VGA_BLUE         1
#define VGA_GREEN        2
#define VGA_CYAN         3
#define VGA_RED          4
#define VGA_MAGENTA      5
#define VGA_BROWN        6
#define VGA_LIGHT_GREY   7
#define VGA_DARK_GREY    8
#define VGA_LIGHT_BLUE   9
#define VGA_LIGHT_GREEN  10
#define VGA_LIGHT_CYAN   11
#define VGA_LIGHT_RED    12
#define VGA_LIGHT_MAGENTA 13
#define VGA_LIGHT_BROWN  14
#define VGA_WHITE        15
#define VGA_YELLOW       16

/**
 * @brief Initialize the console
 */
void console_init(void);

/**
 * @brief Clear the console screen
 */
void console_clear(void);

/**
 * @brief Output a character to the console
 * @param c The character to output
 */
void console_putchar(char c);

/**
 * @brief Output a string to the console
 * @param str The null-terminated string to output
 */
void console_write(const char* str);

/**
 * @brief Output a hexadecimal value to the console
 * @param val The value to output in hexadecimal
 */
void console_write_hex(uint32_t val);

/**
 * @brief Output a 64-bit unsigned decimal value to the console
 * @param val The value to output in decimal
 */
void console_write_dec_u64(uint64_t val);

/**
 * @brief Output a 64-bit signed decimal value to the console
 * @param val The value to output in decimal
 */
void console_write_dec_s64(int64_t val);

/**
 * @brief Output a 32-bit floating-point value to the console
 * @param val The value to output
 */
void console_write_float_f32(float32_t val);

/**
 * @brief Output a 64-bit floating-point value to the console
 * @param val The value to output
 */
void console_write_float_f64(float64_t val);

/**
 * @brief Output a value to the console, automatically selecting the correct function based on the type of value
 * @param value The value to output (can be any integer or floating-point type)
 */
#define console_write_dec(value)                        \
    GENERIC((value),                                    \
        unsigned char:      console_write_dec_u64,      \
        unsigned short:     console_write_dec_u64,      \
        unsigned int:       console_write_dec_u64,      \
        unsigned long:      console_write_dec_u64,      \
        unsigned long long: console_write_dec_u64,      \
        signed char:        console_write_dec_s64,      \
        signed short:       console_write_dec_s64,      \
        signed int:         console_write_dec_s64,      \
        signed long:        console_write_dec_s64,      \
        signed long long:   console_write_dec_s64,      \
        float:              console_write_float_f32,    \
        double:             console_write_float_f64     \
    )((value))

/**
 * @brief Set the console text color
 * @param fg The foreground color
 * @param bg The background color
 */
void console_set_color(uint8_t fg, uint8_t bg);

#endif
