#ifndef SHARED_VIDEO_ABI_H
#define SHARED_VIDEO_ABI_H

#include "types.h"

/**
 * @brief VESA linear framebuffer mode information shared with user-space. \struct vesa_mode_info
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
 * @brief Struct to represent the mouse state \struct mouse_state
 */
struct mouse_state
{
    int32_t x;
    int32_t y;
    uint8_t buttons;
    uint8_t moved;
};

#define MOUSE_LEFT_BUTTON 0x01
#define MOUSE_RIGHT_BUTTON 0x02
#define MOUSE_MIDDLE_BUTTON 0x04

#endif
