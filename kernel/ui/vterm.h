#ifndef KERNEL_VTERM_H
#define KERNEL_VTERM_H

#include "../../shared/types.h"

/**
 * @brief Maximum number of virtual terminals
 */
#define VTERM_MAX_COUNT     4

/**
 * @brief Terminal dimensions
 */
#define VTERM_WIDTH         80
#define VTERM_HEIGHT        25
#define VTERM_SCROLLBACK    200
#define VTERM_BUFFER_SIZE   (VTERM_WIDTH * VTERM_SCROLLBACK)
#define VTERM_VISIBLE_SIZE  (VTERM_WIDTH * VTERM_HEIGHT)

/**
 * @brief Terminal IDs
 */
#define VTERM_CONSOLE       0
#define VTERM_INIT          1
#define VTERM_USER1         2
#define VTERM_USER2         3

/**
 * @brief Virtual terminal structure \struct vterm
 */
struct vterm
{
    uint16_t buffer[VTERM_BUFFER_SIZE];
    uint32_t cursor_row;
    uint32_t cursor_col;
    uint32_t scroll_offset;
    uint32_t total_lines;
    uint8_t color;
    pid_t owner_pid;
    bool active;
    char name[16];
};

/**
 * @brief Initialize the virtual terminal subsystem
 */
void vterm_init(void);

/**
 * @brief Get a virtual terminal by ID
 * @param id Terminal ID (0 to VTERM_MAX_COUNT-1)
 * @return Pointer to the terminal, or NULL if invalid ID
 */
struct vterm* vterm_get(uint8_t id);

/**
 * @brief Get the currently active terminal
 * @return Pointer to the active terminal
 */
struct vterm* vterm_get_active(void);

/**
 * @brief Get the ID of the currently active terminal
 * @return Active terminal ID
 */
uint8_t vterm_get_active_id(void);

/**
 * @brief Switch to a different virtual terminal
 * @param id Terminal ID to switch to
 * @return 0 on success, -1 on error
 */
int vterm_switch(uint8_t id);

/**
 * @brief Write a character to a virtual terminal
 * @param vt Pointer to the virtual terminal
 * @param c Character to write
 */
void vterm_putchar(struct vterm* vt, char c);

/**
 * @brief Write a string to a virtual terminal
 * @param vt Pointer to the virtual terminal
 * @param str Null-terminated string to write
 */
void vterm_write(struct vterm* vt, const char* str);

/**
 * @brief Write a unsigned decimal number to a virtual terminal
 * @param vt Pointer to the virtual terminal
 * @param val Value to write
 */
void vterm_write_dec_u64(struct vterm* vt, uint64_t val);

/**
 * @brief Write a signed decimal number to a virtual terminal
 * @param vt Pointer to the virtual terminal
 * @param val Value to write
 */
void vterm_write_dec_s64(struct vterm* vt, int64_t val);

/**
 * @brief Write a 32-bit floating-point number to a virtual terminal
 * @param vt Pointer to the virtual terminal
 * @param val Value to write
 */
void vterm_write_float_f32(struct vterm* vt, float32_t val);

/**
 * @brief Write a 64-bit floating-point number to a virtual terminal
 * @param vt Pointer to the virtual terminal
 * @param val Value to write
 */
void vterm_write_float_f64(struct vterm* vt, float64_t val);

/**
 * @brief Write a decimal number to a virtual terminal, automatically selecting the correct function based on the type of value
 * @param vt The virtual terminal to write to
 * @param value The value to write (can be any integer type)
 */
#define vterm_write_dec(vt, value)                  \
    GENERIC((value),                                \
        unsigned char:      vterm_write_dec_u64,    \
        unsigned short:     vterm_write_dec_u64,    \
        unsigned int:       vterm_write_dec_u64,    \
        unsigned long:      vterm_write_dec_u64,    \
        unsigned long long: vterm_write_dec_u64,    \
        signed char:        vterm_write_dec_s64,    \
        signed short:       vterm_write_dec_s64,    \
        signed int:         vterm_write_dec_s64,    \
        signed long:        vterm_write_dec_s64,    \
        signed long long:   vterm_write_dec_s64,    \
        double:             vterm_write_float_f64,  \
        float:              vterm_write_float_f32   \
    )((vt), (value))

/**
 * @brief Clear a virtual terminal
 * @param vt Pointer to the virtual terminal
 */
void vterm_clear(struct vterm* vt);

/**
 * @brief Set the color for a virtual terminal
 * @param vt Pointer to the virtual terminal
 * @param fg Foreground color
 * @param bg Background color
 */
void vterm_set_color(struct vterm* vt, uint8_t fg, uint8_t bg);

/**
 * @brief Set the owner process of a virtual terminal
 * @param id Terminal ID
 * @param pid Owner process ID
 */
void vterm_set_owner(uint8_t id, pid_t pid);

/**
 * @brief Get the terminal assigned to a process
 * @param pid Process ID
 * @return Terminal ID, or -1 if none assigned
 */
int vterm_get_by_pid(pid_t pid);

/**
 * @brief Refresh the display with the active terminal's buffer
 */
void vterm_refresh(void);

/**
 * @brief Scroll the terminal view up (PageUp)
 * @param vt Pointer to the virtual terminal
 * @param lines Number of lines to scroll up
 */
void vterm_scroll_up(struct vterm* vt, uint32_t lines);

/**
 * @brief Scroll the terminal view down (PageDown)
 * @param vt Pointer to the virtual terminal
 * @param lines Number of lines to scroll down
 */
void vterm_scroll_down(struct vterm* vt, uint32_t lines);

/**
 * @brief Reset scroll position to bottom (current output)
 * @param vt Pointer to the virtual terminal
 */
void vterm_scroll_reset(struct vterm* vt);

/**
 * @brief Handle keyboard shortcut for terminal switching and scrolling
 * @param scancode Raw keyboard scancode
 * @return true if the scancode was handled as a switch/scroll command
 */
bool vterm_handle_switch(uint8_t scancode);

#endif
