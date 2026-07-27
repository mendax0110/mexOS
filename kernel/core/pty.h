#ifndef KERNEL_CORE_PTY_H
#define KERNEL_CORE_PTY_H

#include "../../shared/types.h"

/**
 * @brief Initializes the pseudo-terminal subsystem.
 */
void pty_init(void);

/**
 * @brief Creates a new pseudo-terminal.
 * @param owner The PID of the process creating the pseudo-terminal.
 * @return The ID of the created pseudo-terminal, or -1 on failure.
 */
int pty_create(pid_t owner);

/**
 * @brief Destroys a pseudo-terminal.
 * @param id The ID of the pseudo-terminal to destroy.
 * @param caller The PID of the process requesting the destruction.
 * @return 0 on success, or -1 on failure.
 */
int pty_destroy(int id, pid_t caller);

/**
 * @brief Attaches a slave pseudo-terminal to a master.
 * @param id The ID of the master pseudo-terminal.
 * @return 0 on success, or -1 on failure.
 */
int pty_attach_slave(int id);

/**
 * @brief Detaches a slave pseudo-terminal from a master.
 * @param id The ID of the master pseudo-terminal.
 * @param buffer The data to read.
 * @param size The size of the data to read.
 * @param caller The PID of the process reading from the master.
 * @return 0 on success, or -1 on failure.
 */
int pty_master_read(int id, char* buffer, uint32_t size, pid_t caller);

/**
 * @brief Writes data to the master pseudo-terminal.
 * @param id The ID of the master pseudo-terminal.
 * @param buffer The data to write.
 * @param size The size of the data to write.
 * @param caller The PID of the process writing to the master.
 * @return The number of bytes written, or -1 on failure.
 */
int pty_master_write(int id, const char* buffer, uint32_t size, pid_t caller);

/**
 * @brief Reads data from the slave pseudo-terminal.
 * @param id The ID of the slave pseudo-terminal.
 * @param buffer The buffer to read data into.
 * @param size The size of the buffer.
 * @return The number of bytes read, or -1 on failure.
 */
int pty_slave_read(int id, char* buffer, uint32_t size);

/**
 * @brief Writes data to the slave pseudo-terminal.
 * @param id The ID of the slave pseudo-terminal.
 * @param buffer The data to write.
 * @param size The size of the data to write.
 * @return The number of bytes written, or -1 on failure.
 */
int pty_slave_write(int id, const char* buffer, uint32_t size);

/**
 * @brief Cleans up all pseudo-terminals associated with a process.
 * @param pid The PID of the process to clean up.
 */
void pty_process_cleanup(pid_t pid);

#endif
