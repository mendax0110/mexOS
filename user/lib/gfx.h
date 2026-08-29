#ifndef USER_GFX_H
#define USER_GFX_H

#include "runtime.h"

/**
 * @brief Scale a color channel value to the appropriate size and position for a pixel format.
 * @param value The Value to scale
 * @param size The size of the color channel
 * @param pos The position of the color channel
 * @return The scaled color channel value
 */
static inline uint32_t gfx_channel(const uint8_t value, const uint8_t size, const uint8_t pos)
{
    if (!size) return 0;
    uint32_t scaled = value;
    if (size < 8) scaled >>= 8 - size;
    if (size > 8) scaled <<= size - 8;
    return scaled << pos;
}

/**
 * @brief Gfx rgb settings
 * @param mode The vesa mode to use for the pixel format
 * @param red The red color channel value
 * @param green The green color channel value
 * @param blue The blue color channel value
 * @return The combined RGB value
 */
static inline uint32_t gfx_rgb(const struct vesa_mode_info* mode, const uint8_t red, const uint8_t green, const uint8_t blue)
{
    return gfx_channel(red, mode->red_size, mode->red_pos) |
           gfx_channel(green, mode->green_size, mode->green_pos) |
           gfx_channel(blue, mode->blue_size, mode->blue_pos);
}

/**
 * @brief Plot a pixel on the screen at the specified coordinates with the given color.
 * @param mode The vesa mode to use for the pixel format
 * @param buffer The buffer to plot the pixel in
 * @param x The x coordinate of the pixel
 * @param y The y coordinate of the pixel
 * @param color The color of the pixel
 */
static inline void gfx_pixel(const struct vesa_mode_info* mode, uint8_t* buffer, const int x, const int y, const uint32_t color)
{
    if (x < 0 || y < 0 || x >= (int)mode->width || y >= (int)mode->height) return;
    const uint32_t bytes = ((uint32_t)mode->bpp + 7U) / 8U;
    uint8_t* pixel = buffer + (uint32_t)y * mode->pitch + (uint32_t)x * bytes;
    pixel[0] = (uint8_t)color;
    if (bytes > 1) pixel[1] = (uint8_t)(color >> 8);
    if (bytes > 2) pixel[2] = (uint8_t)(color >> 16);
    if (bytes == 4) pixel[3] = (uint8_t)(color >> 24);
}

/**
 * @brief Plot a rectangle on the screen at specified coordinates with given color and size
 * @param mode The vesa mode to use for the pixel format
 * @param buffer The buffer to plot the rectangle in
 * @param x The x coordinate of the rectangle
 * @param y The y coordinate of the rectangle
 * @param width The width of the rectangle
 * @param height The height of the rectangle
 * @param color The color of the pixel
 */
static inline void gfx_rect(const struct vesa_mode_info* mode, uint8_t* buffer, int x, int y, int width, int height, const uint32_t color)
{
    if (!mode || !buffer) return;
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > (int)mode->width) width = (int)mode->width - x;
    if (y + height > (int)mode->height) height = (int)mode->height - y;
    if (width <= 0 || height <= 0) return;

    if (mode->bpp == 32)
    {
        for (int row = 0; row < height; row++)
        {
            uint32_t* pixels = (uint32_t*)(buffer + (uint32_t)(y + row) * mode->pitch + (uint32_t)x * 4U);
            for (int column = 0; column < width; column++)
            {
                pixels[column] = color;
            }
        }
        return;
    }

    if (mode->bpp == 24)
    {
        const uint8_t low = (uint8_t)color;
        const uint8_t middle = (uint8_t)(color >> 8);
        const uint8_t high = (uint8_t)(color >> 16);
        for (int row = 0; row < height; row++)
        {
            uint8_t* pixel = buffer + (uint32_t)(y + row) * mode->pitch + (uint32_t)x * 3U;
            for (int column = 0; column < width; column++)
            {
                pixel[0] = low;
                pixel[1] = middle;
                pixel[2] = high;
                pixel += 3;
            }
        }
        return;
    }

    if (mode->bpp == 15 || mode->bpp == 16)
    {
        for (int row = 0; row < height; row++)
        {
            uint16_t* pixels = (uint16_t*)(buffer + (uint32_t)(y + row) * mode->pitch + (uint32_t)x * 2U);
            for (int column = 0; column < width; column++)
            {
                pixels[column] = (uint16_t)color;
            }
        }
    }
}

/**
 * @brief Plots a frame on the screen with given coordinates, size, and color
 * @param mode The vesa mode to use for the pixel format
 * @param buffer The buffer to plot the frame in
 * @param x The x coordinate of the frame
 * @param y The y coordinate of the frame
 * @param width The width of the frame
 * @param height The height of the frame
 * @param color The color of the pixel
 */
static inline void gfx_frame(const struct vesa_mode_info* mode, uint8_t* buffer,
                             const int x, const int y, const int width, const int height,
                             const uint32_t color)
{
    gfx_rect(mode, buffer, x, y, width, 1, color);
    gfx_rect(mode, buffer, x, y + height - 1, width, 1, color);
    gfx_rect(mode, buffer, x, y, 1, height, color);
    gfx_rect(mode, buffer, x + width - 1, y, 1, height, color);
}

static const uint8_t gfx_font[47][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {15,16,16,16,16,16,15}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {15,16,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {31,4,4,4,4,4,31}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,2,12},
    {0,0,0,31,0,0,0}, {0,0,0,0,0,12,12},
    {0,12,12,0,12,12,0}, {1,2,4,8,16,0,0},
    {16,8,4,2,4,8,16}, {1,2,4,8,4,2,1},
    {0,4,4,31,4,4,0}, {0,4,21,14,21,4,0},
    {14,17,21,21,21,16,15}, {0,0,0,0,0,0,31},
    {4, 15, 20, 14, 5, 30, 4}
};

/**
 * @brief Plots a glyph for a given character from the font array
 * @param character The character to plot
 * @return A pointer to the glyph data or NULL if the character is not supported
 */
static inline const uint8_t* gfx_glyph(char character)
{
    if (character >= 'a' && character <= 'z') character -= 32;
    if (character >= 'A' && character <= 'Z') return gfx_font[character - 'A'];
    if (character >= '0' && character <= '9') return gfx_font[26 + character - '0'];
    if (character == '-') return gfx_font[36];
    if (character == '.') return gfx_font[37];
    if (character == ':') return gfx_font[38];
    if (character == '/') return gfx_font[39];
    if (character == '>') return gfx_font[40];
    if (character == '<') return gfx_font[41];
    if (character == '+') return gfx_font[42];
    if (character == '*') return gfx_font[43];
    if (character == '@') return gfx_font[44];
    if (character == '~') return gfx_font[45];
    if (character == '$') return gfx_font[46];
    return NULL;
}

/**
 * @brief Plots a character on the screen
 * @param mode The vesa mode to use for the pixel format
 * @param buffer The buffer to plot the character in
 * @param character The character to plot
 * @param x The x coordinate of the character
 * @param y The y coordinate of the character
 * @param scale The scale of the character
 * @param color The color of the character
 */
static inline void gfx_char(const struct vesa_mode_info* mode, uint8_t* buffer,
                            const char character, const int x, const int y,
                            const int scale, const uint32_t color)
{
    const uint8_t* glyph = gfx_glyph(character);
    if (!glyph) return;
    for (int row = 0; row < 7; row++)
    {
        for (int column = 0; column < 5; column++)
        {
            if (glyph[row] & (1U << (4 - column)))
            {
                gfx_rect(mode, buffer, x + column * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

/**
 * @brief Plots a string of characters on the screen
 * @param mode The vesa mode to use for the pixel format
 * @param buffer The buffer to plot the text in
 * @param text The string to plot
 * @param x The x coordinate of the text
 * @param y The y coordinate of the text
 * @param scale The scale of the text
 * @param color The color of the text
 */
static inline void gfx_text(const struct vesa_mode_info* mode, uint8_t* buffer,
                            const char* text, int x, const int y,
                            const int scale, const uint32_t color)
{
    while (*text)
    {
        gfx_char(mode, buffer, *text++, x, y, scale, color);
        x += 6 * scale;
    }
}

#endif
