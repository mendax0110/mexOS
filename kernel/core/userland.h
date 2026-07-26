#ifndef KERNEL_CORE_USERLAND_H
#define KERNEL_CORE_USERLAND_H

#include "../../shared/types.h"

struct task;

/**
 * @brief Spawns a new userland task.
 * @param path The path to the executable.
 * @param argc The number of arguments.
 * @param argv The arguments array.
 * @param terminal_id The terminal ID to associate with the task.
 * @return A pointer to the created task, or NULL on failure.
 */
struct task* userland_spawn(const char* path, int argc, const char* const argv[], uint8_t terminal_id);

/**
 * @brief Spawns the init process from the initrd.
 * @param terminal_id The terminal ID to associate with the init process.
 * @param desktop_mode Start the desktop when true, otherwise start the shell.
 * @return A pointer to the created init task, or NULL on failure.
 */
struct task* userland_spawn_init(uint8_t terminal_id, bool desktop_mode);

#endif
