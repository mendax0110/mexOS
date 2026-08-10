#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include "../../shared/types.h"
#include "include/source_location.h"
#include "lib/string.h"

/**
 * @brief Simple in-memory logging system
 */
#define LOG_MAX_ENTRIES     2048
#define LOG_MAX_MSG_LEN     1024

/**
 * @brief Identifier for truncated log messages
 */
#define LOG_FLAG_TRUNCATE 0x01

/**
 * @brief Magic number for log storage validation
 */
#define LOG_STORAGE_MAGIC 0x4CF4753u

/**
 * @brief Version number for log storage format
 */
#define LOG_STORAGE_VERSION 1u

/// @brief Log entry structure \struct log_entry
struct log_entry
{
    uint32_t sequence;
    uint32_t timestamp;
    uint8_t level;
    uint8_t flags;
    uint16_t reserved;
    char message[LOG_MAX_MSG_LEN];
};

/**
 * @brief Log levels \enum log_level_t
 */
typedef enum
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
} log_level_t;

/**
 * @brief Log storage structure for persistent logging \struct t_log_storage
 */
typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    uint32_t sequence;
    uint32_t total_written;
    uint32_t dropped;
} t_log_storage;

/**
 * @brief Macro to define log levels and their string representations
 */
#define LOG_LEVELS                \
    X(LOG_LEVEL_DEBUG, "DBG ")    \
    X(LOG_LEVEL_INFO,  "INF ")    \
    X(LOG_LEVEL_WARN,  "WRN ")    \
    X(LOG_LEVEL_ERROR, "ERR ")

/**
 * @brief Convert log level to string
 * @param level The log level
 * @return String representation of the log level
 */
const char* log_level_to_string(log_level_t level);

/**
 * @brief Initialize the logging system
 */
void log_init(void);

/**
 * @brief Write a log entry
 * @param level The log level
 * @param file The source file name
 * @param line The line number in the source file
 * @param msg The log message
 */
void log_write(uint8_t level, const char* file, int line , const char* msg);

/**
 * @brief Write an info log entry
 * @param msg The log message
 */
void log_info(const char* msg);

/**
 * @brief Write a debug log entry
 * @param msg The log message
 */
#define log_debug(msg)              \
{                                   \
    log_write(LOG_LEVEL_DEBUG,      \
                    __FILENAME__,   \
                    __LINE__,       \
                    msg             \
    );                              \
}

/**
 * @brief Write a info log entry
 * @param msg The log message
 */
#define log_info(msg)               \
{                                   \
    log_write(LOG_LEVEL_INFO,       \
                    __FILENAME__,   \
                    __LINE__,       \
                    msg             \
    );                              \
}

/**
 * @brief Write a warning log entry
 * @param msg The log message
 */
#define log_warn(msg)               \
{                                   \
    log_write(LOG_LEVEL_WARN,       \
                    __FILENAME__,   \
                    __LINE__,       \
                    msg             \
    );                              \
}

/**
 * @brief Write an error log entry
 * @param msg The log message
 */
#define log_error(msg)              \
{                                   \
    log_write(LOG_LEVEL_ERROR,      \
                    __FILENAME__,   \
                    __LINE__,       \
                    msg             \
    );                              \
}

/**
 * @brief Get the number of log entries currently stored
 * @return The number of log entries
 */
uint32_t log_get_count(void);

/**
 * @brief Get the total number of log entries written since system start
 * @return The total number of log entries written
 */
uint32_t log_get_total_written(void);

/**
 * @brief Get the number of log entries dropped due to buffer overflow
 * @return The number of log entries dropped
 */
uint32_t log_get_dropped(void);

/**
 * @brief Get log statistics.
 */
void log_stats(void);

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

/**
 * @brief Save the current log entries to a file
 * @param path The path to the file where logs will be saved
 * @return 0 on success, or a negative error code
 */
int log_save(const char* path);

/**
 * @brief Load log entries from a file
 * @param path The path to the file from which logs will be loaded
 * @return 0 on success, or a negative error code
 */
int log_load(const char* path);

/**
 * @brief Write a formatted log info entry
 * @param fmt The format string
 * @param ... Additional arguments for formatting
 */
#define log_info_fmt(fmt, ...)                                  \
{                                                               \
    char buffer[LOG_MAX_MSG_LEN];                               \
    snprintf(buffer, LOG_MAX_MSG_LEN, fmt, ##__VA_ARGS__);      \
    log_write(LOG_LEVEL_INFO, __FILENAME__, __LINE__, buffer);  \
}


/**
 * @brief Write a formatted log warning entry
 * @param fmt The format string
 * @param ... Additional arguments for formatting
 */
#define log_warn_fmt(fmt, ...)                                  \
{                                                               \
    char buffer[LOG_MAX_MSG_LEN];                               \
    snprintf(buffer, LOG_MAX_MSG_LEN, fmt, ##__VA_ARGS__);      \
    log_write(LOG_LEVEL_WARN, __FILENAME__, __LINE__, buffer);  \
}

/**
 * @brief Write a formatted log error entry
 * @param fmt The format string
 * @param ... Additional arguments for formatting
 */
#define log_error_fmt(fmt, ...)                                 \
{                                                               \
    char buffer[LOG_MAX_MSG_LEN];                               \
    snprintf(buffer, LOG_MAX_MSG_LEN, fmt, ##__VA_ARGS__);      \
    log_write(LOG_LEVEL_ERROR, __FILENAME__, __LINE__, buffer); \
}

#endif
