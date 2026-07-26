#ifndef KERNEL_THREAD_H
#define KERNEL_THREAD_H

#include "../shared/types.h"
#include "../shared/compiler.h"

#define THREAD_STACK_SIZE 4096
#define THREAD_MAX_COUNT 1024

/**
 * @brief Struct representing the CPU context of a thread \struct thread_context
 */
typedef struct thread_context
{
    uint32_t eip;
    uint32_t esp;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t esi;
    uint32_t edi;
    uint32_t eflags;
    void* stack_base;
} PACKED thread_context_t;

/**
 * @brief Enum representing the state of a thread \enum thread_state
 */
enum thread_state
{
    THREAD_RUNNING = 0,
    THREAD_READY = 1,
    THREAD_BLOCKED = 2,
    THREAD_ZOMBIE = 3,
} PACKED;

/**
 * @brief X-Macro to translate thread states to strings
 */
#define THREAD_STATES              \
    X(THREAD_RUNNING, "RUNNING ")  \
    X(THREAD_READY,   "READY   ")  \
    X(THREAD_BLOCKED, "BLOCKED ")  \
    X(THREAD_ZOMBIE,  "ZOMBIE  ")

/**
 * @brief Describes the state of a thread as a string
 * @param state The state enum to describe
 * @return A string describing the thread state
 */
const char* thread_state_to_string(enum thread_state state);

/**
 * @brief Getter for the actual state of the given thread
 * @param thread The thread to check
 * @return The enum representing the state of the thread
 */
enum thread_state thread_get_state(void* thread);

/**
 * @brief Creates a new thread with the given entry point and mode
 * @param entry The entry point for the created thread
 * @param kernel_mode The mode of the thread
 * @return A void ptr
 */
void* thread_create(void (*entry)(void), bool kernel_mode);

/**
 * @brief Destroys a given thread
 * @param thread The thread to destroy
 * @return True if destroyed successfully, false otherwise
 */
bool thread_destroy(void* thread);

/**
 * @brief Initializes the thread handler
 */
void thread_init(void);

#endif // KERNEL_THREAD_H
