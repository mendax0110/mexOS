#ifndef KERNEL_EDITOR_H
#define KERNEL_EDITOR_H

#include "include/types.h"

// Editor modes
#define EDITOR_MODE_TEXT    0
#define EDITOR_MODE_BASIC   1
#define EDITOR_MODE_HEX     2

// Editor buffer sizes
#define EDITOR_MAX_FILE_SIZE    4096
#define EDITOR_LINE_SIZE        256
#define EDITOR_MAX_LINES        64

// Editor commands
#define EDITOR_CMD_QUIT         0
#define EDITOR_CMD_SAVE         1
#define EDITOR_CMD_SAVE_QUIT    2
#define EDITOR_CMD_DELETE_LINE  3
#define EDITOR_CMD_PRINT        4
#define EDITOR_CMD_HELP         5
#define EDITOR_CMD_MODE_TEXT    6
#define EDITOR_CMD_MODE_BASIC   7
#define EDITOR_CMD_MODE_HEX     8

/**
 * @brief Editor state structure
 */
typedef struct editor_state
{
    char filename[128];
    char buffer[EDITOR_MAX_FILE_SIZE];
    char line_buffer[EDITOR_LINE_SIZE];
    uint32_t buffer_size;
    uint8_t mode;
    uint8_t modified;
    uint8_t running;
} editor_state_t;

/**
 * @brief Initialize the editor subsystem
 */
void editor_init(void);

/**
 * @brief Open a file in the editor
 * @param filename Name of file to open
 * @param mode Editor mode (EDITOR_MODE_TEXT, EDITOR_MODE_BASIC, EDITOR_MODE_HEX)
 * @return 0 on success, negative on error
 */
int editor_open(const char* filename, uint8_t mode);

/**
 * @brief Run the editor main loop
 */
void editor_run(void);

/**
 * @brief Get current editor state
 * @return Pointer to current editor state
 */
editor_state_t* editor_get_state(void);

/**
 * @brief Execute BASIC interpreter in current buffer
 */
void editor_run_basic(void);

/**
 * @brief List BASIC program from buffer
 */
void editor_list_basic(void);

/**
 * @brief Save current buffer to file
 * @return 0 on success, negative on error
 */
int editor_save(void);

/**
 * @brief Display editor help
 */
void editor_show_help(void);

/**
 * @brief Switch editor mode
 * @param mode New mode to switch to
 */
void editor_set_mode(uint8_t mode);

/**
 * @brief Display text editor interface
 */
void tui_show_editor(void);

#endif
