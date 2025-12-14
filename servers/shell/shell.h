#ifndef KERNEL_SHELL_H
#define KERNEL_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

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
void shell_run(void);

#ifdef __cplusplus
}
#endif

#endif
