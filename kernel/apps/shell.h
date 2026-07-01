#ifndef KERNEL_SHELL_H
#define KERNEL_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "include/types.h"

struct task;

/**
 * @brief Initialize the shell subsystem.
 *
 * This function sets up necessary data structures and
 * prepares the shell for operation.
 */
void shell_init(void);

/**
 * @brief Run the shell main loop.
 *
 * This function starts the shell, displaying the prompt and
 * handling user input until the shell is exited.
 */
_Noreturn void shell_run(void);

/**
 * @brief Execute a shell command.
 *
 * This function takes a command string, parses it, and
 * executes the corresponding shell command.
 *
 * @param cmd The command string to execute.
 */
void execute_command(char* cmd);

/**
 * @brief Spawn the embedded initrd userland process.
 *
 * @param terminal_id Terminal to route the process output to.
 * @return The created task, or NULL on failure.
 */
struct task* shell_spawn_init_process(uint8_t terminal_id);

#ifdef __cplusplus
}
#endif

#endif
