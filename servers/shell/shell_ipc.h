#ifndef SERVERS_SHELL_IPC_H
#define SERVERS_SHELL_IPC_H

#include "../../kernel/include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VGA color codes for console
 */
#define VGA_BLACK           0
#define VGA_BLUE            1
#define VGA_GREEN           2
#define VGA_CYAN            3
#define VGA_RED             4
#define VGA_MAGENTA         5
#define VGA_BROWN           6
#define VGA_LIGHT_GREY      7
#define VGA_DARK_GREY       8
#define VGA_LIGHT_BLUE      9
#define VGA_LIGHT_GREEN     10
#define VGA_LIGHT_CYAN      11
#define VGA_LIGHT_RED       12
#define VGA_LIGHT_MAGENTA   13
#define VGA_YELLOW          14
#define VGA_WHITE           15

/**
 * @brief Special key codes
 */
#define KEY_ARROW_UP    0x80
#define KEY_ARROW_DOWN  0x81
#define KEY_ARROW_LEFT  0x82
#define KEY_ARROW_RIGHT 0x83
#define KEY_HOME        0x84
#define KEY_END         0x85
#define KEY_PAGE_UP     0x86
#define KEY_PAGE_DOWN   0x87
#define KEY_DELETE      0x88
#define KEY_INSERT      0x89
#define KEY_F1          0x8A
#define KEY_F2          0x8B
#define KEY_F3          0x8C
#define KEY_F4          0x8D
#define KEY_F5          0x8E
#define KEY_F6          0x8F
#define KEY_F7          0x90
#define KEY_F8          0x91
#define KEY_F9          0x92
#define KEY_F10         0x93
#define KEY_F11         0x94
#define KEY_F12         0x95

/**
 * @brief Filesystem error codes
 */
#define FS_ERR_OK        0
#define FS_ERR_NOT_FOUND -1
#define FS_ERR_EXISTS    -2
#define FS_ERR_FULL      -3
#define FS_ERR_NOT_DIR   -4
#define FS_ERR_IS_DIR    -5
#define FS_ERR_NOT_EMPTY -6
#define FS_ERR_INVALID   -7
#define FS_ERR_IO        -8

/**
 * @brief Maximum file size
 */
#define FS_MAX_FILE_SIZE 8192

/**
 * @brief Virtual terminal definitions
 */
#define VTERM_MAX_COUNT 4
#define VTERM_SHELL     0
#define VTERM_INIT      1

/**
 * @brief Task states
 */
enum task_state
{
    TASK_RUNNING = 0,
    TASK_READY = 1,
    TASK_BLOCKED = 2,
    TASK_ZOMBIE = 3
};

/**
 * @brief Task structure (minimal for shell display)
 */
struct task
{
    uint32_t id;
    uint32_t pid;
    enum task_state state;
    uint8_t priority;
    uint32_t cpu_ticks;
    struct task *next;
};

/**
 * @brief Virtual terminal structure (minimal)
 */
struct vterm
{
    char name[16];
    int32_t owner_pid;
    int active;
};

/**
 * @brief Initialize shell IPC subsystem
 * @return 0 on success, -1 on failure
 */
int shell_ipc_init(void);

/**
 * @brief Write string to console
 * @param str String to write
 */
void console_write(const char *str);

/**
 * @brief Write character to console
 * @param c Character to write
 */
void console_putchar(char c);

/**
 * @brief Write decimal number to console
 * @param num Number to write
 */
void console_write_dec(int32_t num);

/**
 * @brief Write hexadecimal number to console
 * @param num Number to write
 */
void console_write_hex(uint32_t num);

/**
 * @brief Clear the console
 */
void console_clear(void);

/**
 * @brief Set console foreground and background color
 * @param fg Foreground color
 * @param bg Background color
 */
void console_set_color(uint8_t fg, uint8_t bg);

/**
 * @brief Get console size
 * @param width Pointer to store width
 * @param height Pointer to store height
 * @return 0 on success
 */
int console_get_size(uint16_t *width, uint16_t *height);

/**
 * @brief Set cursor position
 * @param x Column
 * @param y Row
 */
void console_set_pos(uint16_t x, uint16_t y);

/**
 * @brief Get cursor position
 * @param x Pointer to store column
 * @param y Pointer to store row
 */
void console_get_pos(uint16_t *x, uint16_t *y);

/**
 * @brief Read a character from input (blocking)
 * @return Character read
 */
char keyboard_getchar(void);

/**
 * @brief Check if input is available
 * @return Number of events pending
 */
int keyboard_poll(void);

/**
 * @brief Get system uptime in ticks
 * @return Ticks since boot
 */
uint32_t timer_get_ticks(void);

/**
 * @brief Wait for specified number of milliseconds
 * @param ms Milliseconds to wait
 */
void timer_wait(uint32_t ms);

/**
 * @brief Read file contents
 * @param path File path
 * @param buffer Buffer to store content
 * @param size Maximum size to read
 * @return Number of bytes read, or -1 on error
 */
int32_t fs_read(const char *path, void *buffer, uint32_t size);

/**
 * @brief Write file contents
 * @param path File path
 * @param buffer Data to write
 * @param size Size of data
 * @return Number of bytes written, or -1 on error
 */
int32_t fs_write(const char *path, const void *buffer, uint32_t size);

/**
 * @brief Check if file or directory exists
 * @param path Path to check
 * @return 1 if exists, 0 otherwise
 */
int fs_exists(const char *path);

/**
 * @brief Check if path is a directory
 * @param path Path to check
 * @return 1 if directory, 0 otherwise
 */
int fs_is_dir(const char *path);

/**
 * @brief Create a new file
 * @param path File path
 * @return 0 on success
 */
int fs_create_file(const char *path);

/**
 * @brief Create a new directory
 * @param path Directory path
 * @return 0 on success
 */
int fs_create_dir(const char *path);

/**
 * @brief Remove file or directory
 * @param path Path to remove
 * @return 0 on success
 */
int fs_remove(const char *path);

/**
 * @brief List directory contents
 * @param path Directory path
 * @param callback Function called for each entry (name, is_dir)
 * @return Number of entries, or -1 on error
 */
int fs_list_dir(const char *path, void (*callback)(const char *name, int is_dir));

/**
 * @brief Change current directory
 * @param path New directory path
 * @return 0 on success
 */
int fs_change_dir(const char *path);

/**
 * @brief Get current working directory
 * @return Pointer to current directory path
 */
const char *fs_get_cwd(void);

/**
 * @brief Initialize filesystem
 */
void fs_init(void);

/**
 * @brief Clear filesystem cache
 */
void fs_clear_cache(void);

/**
 * @brief Sync filesystem to disk
 */
void fs_sync(void);

/**
 * @brief Check if disk is enabled
 * @return 1 if enabled, 0 otherwise
 */
int fs_is_disk_enabled(void);

/**
 * @brief Enable disk filesystem
 * @param enable 1 to enable, 0 to disable
 */
void fs_enable_disk(int enable);

#ifdef __cplusplus
}
#endif

#endif // SERVERS_SHELL_IPC_H
