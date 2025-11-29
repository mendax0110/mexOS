#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include "../include/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple in-memory logging system
 */
#define LOG_MAX_ENTRIES     64
#define LOG_MAX_MSG_LEN     64

#define LOG_LEVEL_DEBUG     0
#define LOG_LEVEL_INFO      1
#define LOG_LEVEL_WARN      2
#define LOG_LEVEL_ERROR     3

/// @brief Log entry structure \struct log_entry
struct log_entry
{
    uint32_t timestamp;
    uint8_t level;
    char message[LOG_MAX_MSG_LEN];
};

/**
 * @brief Initialize the logging system
 */
void log_init(void);

/**
 * @brief Write a log entry
 * @param level The log level
 * @param msg The log message
 */
void log_write(uint8_t level, const char* msg);

/**
 * @brief Write a debug log entry
 * @param msg The log message
 */
void log_debug(const char* msg);

/**
 * @brief Write an info log entry
 * @param msg The log message
 */
void log_info(const char* msg);

/**
 * @brief Write a warning log entry
 * @param msg The log message
 */
void log_warn(const char* msg);

/**
 * @brief Write an error log entry
 * @param msg The log message
 */
void log_error(const char* msg);

/**
 * @brief Get the number of log entries
 * @return The number of log entries
 */
uint32_t log_get_count(void);

/**
 * @brief Get a log entry by index
 * @param index The index of the log entry
 * @return Pointer to the log entry, or NULL if index is out of bounds
 */
const struct log_entry* log_get_entry(uint32_t index);

/**
 * @brief Clear all log entries
 */
void log_clear(void);

/**
 * @brief Dump all log entries to the console
 */
void log_dump(void);

#ifdef __cplusplus
}
#endif

#endif
