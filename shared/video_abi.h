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

#endif
