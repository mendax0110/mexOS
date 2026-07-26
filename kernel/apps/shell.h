#ifndef KERNEL_SHELL_H
#define KERNEL_SHELL_H

#include "../../shared/types.h"

struct task;

/**
 * @brief Function pointer type for command handlers
 * @param argc Argument count
 * @param argv Argument vector (array of strings)
 */
typedef void (*cmd_handler_fn)(int argc, char** argv);

/**
 * @brief Flags for shell commands \enum cmd_flags_t
 */
typedef enum
{
    CMD_FLAG_NONE = 0,
    CMD_FLAG_ADMIN = 1 << 0,
    CMD_FLAG_OVERRIDABLE = 1 << 1,
} cmd_flags_t;

/**
 * @brief Structure representing a shell command entry \struct cmd_entry_t
 */
typedef struct
{
    const char* name;
    cmd_handler_fn handler;
    cmd_flags_t flags;
} cmd_entry_t;

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
NORETURN void shell_run(void);

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

#endif
