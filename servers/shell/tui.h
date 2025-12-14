#ifndef KERNEL_TUI_H
#define KERNEL_TUI_H

#include "include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Text User Interface (TUI) framework for mexOS
 *
 * Provides panel/window management, drawing utilities, and UI components
 * for building terminal-based user interfaces.
 */

#define TUI_MAX_PANELS 8
#define TUI_BORDER_SINGLE 0
#define TUI_BORDER_DOUBLE 1

/// @brief Panel structure \struct tui_panel
struct tui_panel
{
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    uint8_t border_style;
    uint8_t fg_color;
    uint8_t bg_color;
    char title[64];
    bool visible;
};

/// @brief Progress bar structure \struct tui_progress_bar
struct tui_progress_bar
{
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t value;
    uint8_t fg_color;
    uint8_t bg_color;
    char label[32];
};

/**
 * @brief Initialize TUI framework
 */
void tui_init(void);

/**
 * @brief Create a new panel
 * @param x X position (column)
 * @param y Y position (row)
 * @param width Width in characters
 * @param height Height in characters
 * @param title Panel title
 * @return Panel ID, or -1 on failure
 */
int tui_create_panel(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const char* title);

/**
 * @brief Draw a panel
 * @param panel_id Panel ID to draw
 */
void tui_draw_panel(int panel_id);

/**
 * @brief Clear panel contents (preserve border)
 * @param panel_id Panel ID to clear
 */
void tui_clear_panel(int panel_id);

/**
 * @brief Write text inside a panel
 * @param panel_id Panel ID
 * @param x X offset from panel's left border
 * @param y Y offset from panel's top border
 * @param text Text to write
 */
void tui_panel_write(int panel_id, uint8_t x, uint8_t y, const char* text);

/**
 * @brief Set panel colors
 * @param panel_id Panel ID
 * @param fg Foreground color
 * @param bg Background color
 */
void tui_set_panel_colors(int panel_id, uint8_t fg, uint8_t bg);

/**
 * @brief Draw a horizontal line
 * @param x X position
 * @param y Y position
 * @param length Length in characters
 * @param style Border style
 */
void tui_draw_hline(uint8_t x, uint8_t y, uint8_t length, uint8_t style);

/**
 * @brief Draw a vertical line
 * @param x X position
 * @param y Y position
 * @param length Length in characters
 * @param style Border style
 */
void tui_draw_vline(uint8_t x, uint8_t y, uint8_t length, uint8_t style);

/**
 * @brief Draw a progress bar
 * @param bar Progress bar structure
 */
void tui_draw_progress_bar(const struct tui_progress_bar* bar);

/**
 * @brief Draw a status bar at bottom of screen
 * @param text Status text
 */
void tui_draw_status_bar(const char* text);

/**
 * @brief Draw the system dashboard
 */
void tui_draw_dashboard(void);

/**
 * @brief Update dashboard with current system stats
 */
void tui_update_dashboard(void);

/**
 * @brief Show log viewer interface
 */
void tui_show_log_viewer(void);


/**
 * @brief Show file browser interface
 * @param path Initial path to display
 */
void tui_show_file_browser(const char* path);

/**
 * @brief Show task manager interface
 */
void tui_show_task_manager(void);

/**
 * @brief Show memory monitor interface
 */
void tui_show_memory_monitor(void);

/**
 * @brief Run the TUI application loop
 */
void tui_run_app(void);

#ifdef __cplusplus
}
#endif

#endif
