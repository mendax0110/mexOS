#ifndef KERNEL_THREAD_H
#define KERNEL_THREAD_H

#include "include/types.h"
#include "include/cast.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THREAD_STACK_SIZE 4096
#define THREAD_MAX_COUNT 1024

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

enum thread_state
{
    THREAD_RUNNING = 0,
    THREAD_READY = 1,
    THREAD_BLOCKED = 2,
    THREAD_ZOMBIE = 3,
} PACKED;

#define THREAD_STATES              \
    X(THREAD_RUNNING, "RUNNING ")  \
    X(THREAD_READY,   "READY   ")  \
    X(THREAD_BLOCKED, "BLOCKED ")  \
    X(THREAD_ZOMBIE,  "ZOMBIE  ")

const char* thread_state_to_string(enum thread_state state);

enum thread_state thread_get_state(void* thread);

void* thread_create(void (*entry)(void), const bool kernel_mode);

bool thread_destroy(void* thread);

void thread_init(void);






#ifdef __cplusplus
};
#endif

#endif // KERNEL_THREAD_H