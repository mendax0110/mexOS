#include "vesa.h"
#include "lib/log.h"
#include "mm/vmm.h"
#include "lib/string.h"
#include "include/addr.h"
#include "ui/console.h"

static struct vesa_mode_info current_mode;
static bool vesa_available = false;
static uint8_t* framebuffer_ptr = NULL;

#define VESA_MAX_WIDTH 8192U
#define VESA_MAX_HEIGHT 8192U
#define VESA_MAX_FRAMEBUFFER_BYTES (64U * 1024U * 1024U)

/**
 * @brief Multiboot framebuffer information structure \struct multiboot_framebuffer
 */
struct multiboot_framebuffer
{
    uint32_t framebuffer_addr_low;
    uint32_t framebuffer_addr_high;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
} PACKED;

static bool map_framebuffer_pages(void)
{
    page_directory_t* page_dir = vmm_get_current_directory();
    if (!page_dir)
    {
        return false;
    }

    const uint32_t phys_base = current_mode.framebuffer & ~(PAGE_SIZE - 1U);
    const uint32_t page_offset = current_mode.framebuffer - phys_base;
    if (current_mode.framebuffer_size > ~0U - page_offset)
    {
        return false;
    }

    const uint32_t span = current_mode.framebuffer_size + page_offset;
    if (span > ~0U - (PAGE_SIZE - 1U))
    {
        return false;
    }

    const uint32_t map_bytes = (span + PAGE_SIZE - 1U) & ~(PAGE_SIZE - 1U);
    for (uint32_t offset = 0; offset < map_bytes; offset += PAGE_SIZE)
    {
        if (vmm_map_page(page_dir,
                         phys_base + offset,
                         phys_base + offset,
                         PAGE_PRESENT | PAGE_WRITE | PAGE_CACHE_DISABLE) != 0)
        {
            return false;
        }
    }

    framebuffer_ptr = PTR_FROM_U32(current_mode.framebuffer);
    return true;
}

void vesa_init(void* mboot_info)
{
    log_info("Initializing framebuffer driver");
    memset(&current_mode, 0, sizeof(current_mode));
    framebuffer_ptr = NULL;
    vesa_available = false;

    if (!mboot_info)
    {
        log_warn_fmt("No multiboot info provided (0x%x)", mboot_info);
        return;
    }

    uint32_t* mb = mboot_info;
    const uint32_t flags = mb[0];

    if ((flags & (1 << 12)) == 0)
    {
        log_warn("No framebuffer information in multiboot");
        return;
    }

    const struct multiboot_framebuffer* fb = (struct multiboot_framebuffer*)&mb[22];

    if (fb->framebuffer_type != 1)
    {
        log_warn_fmt("Unsupported framebuffer type: %d", fb->framebuffer_type);
        return;
    }

    if (fb->framebuffer_addr_high != 0 || fb->framebuffer_addr_low == 0)
    {
        log_warn("Framebuffer address is outside the 32-bit physical address space");
        return;
    }

    if (fb->framebuffer_width == 0 || fb->framebuffer_height == 0 ||
        fb->framebuffer_width > VESA_MAX_WIDTH ||
        fb->framebuffer_height > VESA_MAX_HEIGHT)
    {
        log_warn_fmt("Invalid framebuffer dimensions: %dx%d",
                     fb->framebuffer_width, fb->framebuffer_height);
        return;
    }

    if (fb->framebuffer_bpp != 15 && fb->framebuffer_bpp != 16 &&
        fb->framebuffer_bpp != 24 && fb->framebuffer_bpp != 32)
    {
        log_warn_fmt("Unsupported framebuffer depth: %d", fb->framebuffer_bpp);
        return;
    }

    const uint32_t bytes_per_pixel = (fb->framebuffer_bpp + 7U) / 8U;
    if (fb->framebuffer_width > ~0U / bytes_per_pixel)
    {
        log_warn("Framebuffer row size overflows");
        return;
    }

    const uint32_t minimum_pitch = fb->framebuffer_width * bytes_per_pixel;
    if (fb->framebuffer_pitch < minimum_pitch ||
        fb->framebuffer_height > ~0U / fb->framebuffer_pitch)
    {
        log_warn_fmt("Invalid framebuffer pitch: %d", fb->framebuffer_pitch);
        return;
    }

    const uint32_t framebuffer_size =
        fb->framebuffer_pitch * fb->framebuffer_height;
    if (framebuffer_size == 0 ||
        framebuffer_size > VESA_MAX_FRAMEBUFFER_BYTES ||
        fb->framebuffer_addr_low > ~0U - framebuffer_size)
    {
        log_warn_fmt("Invalid framebuffer size: %d", framebuffer_size);
        return;
    }

    current_mode.width = fb->framebuffer_width;
    current_mode.height = fb->framebuffer_height;
    current_mode.pitch = fb->framebuffer_pitch;
    current_mode.bpp = fb->framebuffer_bpp;
    current_mode.type = fb->framebuffer_type;
    current_mode.framebuffer = fb->framebuffer_addr_low;
    current_mode.framebuffer_size = framebuffer_size;

    current_mode.red_pos = fb->color_info[0];
    current_mode.red_size = fb->color_info[1];
    current_mode.green_pos = fb->color_info[2];
    current_mode.green_size = fb->color_info[3];
    current_mode.blue_pos = fb->color_info[4];
    current_mode.blue_size = fb->color_info[5];

    const bool masks_valid =
        current_mode.red_size > 0 &&
        current_mode.green_size > 0 &&
        current_mode.blue_size > 0 &&
        current_mode.red_pos + current_mode.red_size <= current_mode.bpp &&
        current_mode.green_pos + current_mode.green_size <= current_mode.bpp &&
        current_mode.blue_pos + current_mode.blue_size <= current_mode.bpp;

    if (!masks_valid)
    {
        log_warn("Multiboot color masks invalid, assuming standard BGR layout");
        if (current_mode.bpp == 15)
        {
            current_mode.red_pos = 10;
            current_mode.red_size = 5;
            current_mode.green_pos = 5;
            current_mode.green_size = 5;
            current_mode.blue_pos = 0;
            current_mode.blue_size = 5;
        }
        else if (current_mode.bpp == 16)
        {
            current_mode.red_pos = 11;
            current_mode.red_size = 5;
            current_mode.green_pos = 5;
            current_mode.green_size = 6;
            current_mode.blue_pos = 0;
            current_mode.blue_size = 5;
        }
        else
        {
            current_mode.red_pos = 16;
            current_mode.red_size = 8;
            current_mode.green_pos = 8;
            current_mode.green_size = 8;
            current_mode.blue_pos = 0;
            current_mode.blue_size = 8;
        }
    }

    if (!map_framebuffer_pages())
    {
        log_warn("Failed to map framebuffer");
        memset(&current_mode, 0, sizeof(current_mode));
        framebuffer_ptr = NULL;
        return;
    }

    vesa_available = true;

    log_info_fmt("Framebuffer at 0x%x, %dx%d, %d bpp, pitch %d",
                 current_mode.framebuffer, current_mode.width, current_mode.height,
                 current_mode.bpp, current_mode.pitch);
    log_info_fmt("colors: r(pos=%d,size=%d) g(pos=%d,size=%d) b(pos=%d,size=%d)",
                 current_mode.red_pos, current_mode.red_size,
                 current_mode.green_pos, current_mode.green_size,
                 current_mode.blue_pos, current_mode.blue_size);
}

bool vesa_is_available(void)
{
    return vesa_available;
}

bool vesa_get_mode_info(struct vesa_mode_info* info)
{
    if (!vesa_available || !info)
    {
        return false;
    }

    memcpy(info, &current_mode, sizeof(struct vesa_mode_info));
    return true;
}

uint32_t vesa_get_framebuffer(void)
{
    return current_mode.framebuffer;
}

uint32_t vesa_get_framebuffer_size(void)
{
    return current_mode.framebuffer_size;
}

void vesa_plot_pixel(const uint32_t x, const uint32_t y, const uint32_t color)
{
    if (!vesa_available || x >= current_mode.width || y >= current_mode.height)
    {
        return;
    }

    const uint32_t bytes_per_pixel = ((uint32_t)current_mode.bpp + 7U) / 8U;
    const uint32_t offset = y * current_mode.pitch + x * bytes_per_pixel;

    if (current_mode.bpp == 32)
    {
        *((uint32_t*)(framebuffer_ptr + offset)) = color;
    }
    else if (current_mode.bpp == 24)
    {
        framebuffer_ptr[offset + 0] = (color >> 0) & 0xFF;
        framebuffer_ptr[offset + 1] = (color >> 8) & 0xFF;
        framebuffer_ptr[offset + 2] = (color >> 16) & 0xFF;
    }
    else if (current_mode.bpp == 15 || current_mode.bpp == 16)
    {
        *((uint16_t*)(framebuffer_ptr + offset)) = (uint16_t)color;
    }
}

void vesa_draw_hline(uint32_t x1, uint32_t x2, const uint32_t y, const uint32_t color)
{
    if (x1 > x2)
    {
        const uint32_t temp = x1;
        x1 = x2;
        x2 = temp;
    }

    for (uint32_t x = x1; x <= x2; x++)
    {
        vesa_plot_pixel(x, y, color);
    }
}

void vesa_draw_vline(const uint32_t x, uint32_t y1, uint32_t y2, const uint32_t color)
{
    if (y1 > y2)
    {
        const uint32_t temp = y1;
        y1 = y2;
        y2 = temp;
    }

    for (uint32_t y = y1; y <= y2; y++)
    {
        vesa_plot_pixel(x, y, color);
    }
}

void vesa_draw_rect(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height, const uint32_t color)
{
    for (uint32_t row = 0; row < height; row++)
    {
        for (uint32_t col = 0; col < width; col++)
        {
            vesa_plot_pixel(x + col, y + row, color);
        }
    }
}

void vesa_clear_screen(const uint32_t color)
{
    if (!vesa_available)
    {
        return;
    }

    if (current_mode.bpp == 32)
    {
        uint32_t* fb = (uint32_t*)framebuffer_ptr;
        const uint32_t pixels = (current_mode.pitch * current_mode.height) / 4;
        for (uint32_t i = 0; i < pixels; i++)
        {
            fb[i] = color;
        }
    }
    else
    {
        for (uint32_t y = 0; y < current_mode.height; y++)
        {
            for (uint32_t x = 0; x < current_mode.width; x++)
            {
                vesa_plot_pixel(x, y, color);
            }
        }
    }
}

uint32_t vesa_rgb(const uint8_t r, const uint8_t g, const uint8_t b)
{
    return (r << current_mode.red_pos) |
           (g << current_mode.green_pos) |
           (b << current_mode.blue_pos);
}

uint32_t vesa_get_width(void)
{
    return current_mode.width;
}

uint32_t vesa_get_height(void)
{
    return current_mode.height;
}

uint8_t vesa_get_bpp(void)
{
    return current_mode.bpp;
}

void vesa_shutdown(void)
{
    vesa_available = false;
    framebuffer_ptr = NULL;
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: vesa driver shutdown complete\n", __FUNCTION__);
    console_write(msg);
}
