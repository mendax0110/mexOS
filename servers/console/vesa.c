#include "vesa.h"
#include "../../shared/log.h"
#include "../../shared/string.h"
#include "include/cast.h"

#ifdef __KERNEL__
#include "mm/vmm.h"
#else
#include "../../servers/lib/memory.h"
#endif

static struct vesa_mode_info current_mode;
static bool vesa_available = false;
static uint8_t* framebuffer_ptr = NULL;

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
} __attribute__((packed));

void vesa_init(void* mboot_info)
{
    log_info("VESA: Initializing framebuffer driver");

    if (!mboot_info)
    {
        log_warn("VESA: No multiboot info provided");
        return;
    }

    uint32_t* mb = (uint32_t*)mboot_info;
    const uint32_t flags = mb[0];

    if ((flags & (1 << 12)) == 0)
    {
        log_warn("VESA: No framebuffer information in multiboot");
        return;
    }

    const struct multiboot_framebuffer* fb = (struct multiboot_framebuffer*)&mb[22];

    if (fb->framebuffer_type != 1)
    {
        log_warn_fmt("VESA: Unsupported framebuffer type: %d", fb->framebuffer_type);
        return;
    }

    current_mode.width = fb->framebuffer_width;
    current_mode.height = fb->framebuffer_height;
    current_mode.pitch = fb->framebuffer_pitch;
    current_mode.bpp = fb->framebuffer_bpp;
    current_mode.type = fb->framebuffer_type;
    current_mode.framebuffer = fb->framebuffer_addr_low;
    current_mode.framebuffer_size = fb->framebuffer_pitch * fb->framebuffer_height;

    current_mode.red_pos = fb->color_info[0];
    current_mode.red_size = fb->color_info[1];
    current_mode.green_pos = fb->color_info[2];
    current_mode.green_size = fb->color_info[3];
    current_mode.blue_pos = fb->color_info[4];
    current_mode.blue_size = fb->color_info[5];

#ifdef __KERNEL__
    page_directory_t* page_dir = vmm_get_current_directory();
    const uint32_t fb_pages = (current_mode.framebuffer_size + 0xFFF) / 0x1000;
    for (uint32_t i = 0; i < fb_pages; i++)
    {
        const uint32_t virt = current_mode.framebuffer + (i * 0x1000);
        const uint32_t phys = current_mode.framebuffer + (i * 0x1000);
        vmm_map_page(page_dir, virt, phys, PAGE_PRESENT | PAGE_WRITE | PAGE_CACHE_DISABLE);
    }
    framebuffer_ptr = (uint8_t*)(uintptr_t)current_mode.framebuffer;
#else
    void* mapped = mem_map_phys(current_mode.framebuffer, current_mode.framebuffer_size,
                                MEM_PROT_READ | MEM_PROT_WRITE, MEM_FLAG_DEVICE);
    if (!mapped)
    {
        log_error("VESA: Failed to map framebuffer");
        return;
    }
    framebuffer_ptr = (uint8_t*)mapped;
    current_mode.framebuffer = (uint32_t)(uintptr_t)mapped;
#endif

    vesa_available = true;

    log_info_fmt("VESA: Framebuffer at 0x%x, %dx%d, %d bpp, pitch %d",
                 current_mode.framebuffer, current_mode.width, current_mode.height,
                 current_mode.bpp, current_mode.pitch);
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

    const uint32_t offset = y * current_mode.pitch + x * (current_mode.bpp / 8);

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
