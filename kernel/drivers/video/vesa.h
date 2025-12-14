#ifndef KERNEL_VESA_H
#define KERNEL_VESA_H

#include "../../include/types.h"

#define VESA_MAX_MODES      64

/**
 * @brief Video mode information structure
 */
struct vesa_mode_info
{
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
    uint8_t type;
    uint32_t framebuffer;
    uint32_t framebuffer_size;
    uint8_t red_pos;
    uint8_t red_size;
    uint8_t green_pos;
    uint8_t green_size;
    uint8_t blue_pos;
    uint8_t blue_size;
};

/**
 * @brief Initialize VESA/VBE framebuffer driver from multiboot info
 *
 * @param mboot_info Pointer to multiboot information structure
 */
void vesa_init(void* mboot_info);

/**
 * @brief Check if VESA framebuffer is available
 *
 * @return bool true if framebuffer available, false otherwise
 */
bool vesa_is_available(void);

/**
 * @brief Get current video mode information
 *
 * @param info Pointer to vesa_mode_info structure to fill
 * @return bool true if successful, false if no framebuffer
 */
bool vesa_get_mode_info(struct vesa_mode_info* info);

/**
 * @brief Get framebuffer physical address
 *
 * @return uint32_t Physical address of framebuffer, 0 if unavailable
 */
uint32_t vesa_get_framebuffer(void);

/**
 * @brief Get framebuffer size in bytes
 *
 * @return uint32_t Size of framebuffer in bytes
 */
uint32_t vesa_get_framebuffer_size(void);

/**
 * @brief Plot a pixel at specified coordinates
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param color 32-bit ARGB color value
 */
void vesa_plot_pixel(uint32_t x, uint32_t y, uint32_t color);

/**
 * @brief Draw horizontal line
 *
 * @param x1 Starting X coordinate
 * @param x2 Ending X coordinate
 * @param y Y coordinate
 * @param color 32-bit ARGB color value
 */
void vesa_draw_hline(uint32_t x1, uint32_t x2, uint32_t y, uint32_t color);

/**
 * @brief Draw vertical line
 *
 * @param x X coordinate
 * @param y1 Starting Y coordinate
 * @param y2 Ending Y coordinate
 * @param color 32-bit ARGB color value
 */
void vesa_draw_vline(uint32_t x, uint32_t y1, uint32_t y2, uint32_t color);

/**
 * @brief Draw filled rectangle
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color 32-bit ARGB color value
 */
void vesa_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

/**
 * @brief Clear entire screen with color
 *
 * @param color 32-bit ARGB color value
 */
void vesa_clear_screen(uint32_t color);

/**
 * @brief Convert RGB to 32-bit color value
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return uint32_t 32-bit color value
 */
uint32_t vesa_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Get screen width
 *
 * @return uint32_t Width in pixels
 */
uint32_t vesa_get_width(void);

/**
 * @brief Get screen height
 *
 * @return uint32_t Height in pixels
 */
uint32_t vesa_get_height(void);

/**
 * @brief Get bits per pixel
 *
 * @return uint8_t Bits per pixel (typically 24 or 32)
 */
uint8_t vesa_get_bpp(void);

#endif//KERNEL_VESA_H
