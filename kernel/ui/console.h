#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief Output a decimal value to the console
 * @param val The value to output in decimal
 */
void console_write_dec(uint32_t val);

/**
 * @brief Set the console text color
 * @param fg The foreground color
 * @param bg The background color
 */
void console_set_color(uint8_t fg, uint8_t bg);

#ifdef __cplusplus
}
#endif

#endif
