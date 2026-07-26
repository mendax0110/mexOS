#ifndef KERNEL_CORE_SHM_H
#define KERNEL_CORE_SHM_H

#include "../../shared/types.h"

/**
 * @brief Initializes the shared memory subsystem.
 */
void shm_init(void);

/**
 * @brief Creates a new shared memory segment.
 * @param size The size of the shared memory segment in bytes.
 * @param owner The PID of the process creating the segment.
 * @return The ID of the created shared memory segment, or -1 on failure.
 */
int shm_create(uint32_t size, pid_t owner);

/**
 * @brief Maps a shared memory segment into the address space of a process.
 * @param id The ID of the shared memory segment to map.
 * @param pid The PID of the process to map the segment into.
 * @return The virtual address of the mapped segment, or 0 on failure.
 */
uint32_t shm_map(int id, pid_t pid);

/**
 * @brief Detaches a shared memory segment from a process.
 * @param id The ID of the shared memory segment to detach.
 * @param pid The PID of the process to detach the segment from.
 * @return 0 on success, or -1 on failure.
 */
int shm_detach(int id, pid_t pid);

/**
 * @brief Destroys a shared memory segment.
 * @param id The ID of the shared memory segment to destroy.
 * @param owner The PID of the process that owns the segment.
 * @return 0 on success, or -1 on failure.
 */
int shm_destroy(int id, pid_t owner);

/**
 * @brief Cleans up all shared memory segments associated with a process.
 * @param pid The PID of the process to clean up.
 */
void shm_process_cleanup(pid_t pid);

/**
 * @brief Handles shared memory segments when a process forks.
 * @param parent The PID of the parent process.
 * @param child The PID of the child process.
 */
void shm_process_fork(pid_t parent, pid_t child);

#endif
