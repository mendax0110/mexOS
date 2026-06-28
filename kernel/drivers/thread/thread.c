#include "thread.h"
#include "lib/string.h"
#include "mm/heap.h"
#include "mm/alloc_track.h"

static volatile uint32_t thread_count = 0;
static volatile uint32_t max_thread_count = THREAD_MAX_COUNT;
static thread_context_t* threads[THREAD_MAX_COUNT] = {0};

void thread_init(void)
{
    TRY_CTX(__FUNCTION__, NULL)
    {
        thread_count = 0;
        max_thread_count = THREAD_MAX_COUNT;
        memset(threads, 0, sizeof(threads));
    }
}

const char* thread_state_to_string(enum thread_state state)
{
    switch (state)
    {
#define X(state_enum, state_str) case state_enum: return state_str;
        THREAD_STATES
#undef X
        default: return "UNKNOWN";
    }
}

enum thread_state thread_get_state(void* thread)
{
    if (!thread)
    {
        return THREAD_ZOMBIE;
    }

    const thread_context_t* ctx = (thread_context_t*)thread;

    if (ctx->eip == 0 && ctx->esp == 0)
    {
        return THREAD_ZOMBIE;
    }

    return THREAD_READY;
}

void* thread_create(void (*entry)(void), const bool kernel_mode)
{
    UNUSED(kernel_mode); // TODO AdrGos -> implement support for user mode

    if (thread_count >= max_thread_count)
    {
        return NULL;
    }

    thread_context_t* ctx = kmalloc(sizeof(thread_context_t));
    if (!ctx)
    {
        return NULL;
    }

    memset(ctx, 0, sizeof(thread_context_t));

    void* stack = kmalloc(THREAD_STACK_SIZE);
    if (!stack)
    {
        kfree(ctx);
        return NULL;
    }

    ctx->stack_base = stack;
    ctx->eip = FUNC_PTR_TO_U32(entry);
    ctx->esp = PTR_TO_U32(PTR_ARITH(uint8_t, stack, THREAD_STACK_SIZE - 4));
    ctx->ebp = ctx->esp;

    threads[thread_count++] = ctx;

    if (ctx)
    {
        TRACK_ADD(ctx, sizeof(thread_context_t), ALLOC_SRC_THREAD_CONTEXT);
    }
    return ctx;
}

bool thread_destroy(void* thread)
{
    if (!thread)
    {
        return false;
    }

    thread_context_t* ctx = thread;

    for (uint32_t i = 0; i < thread_count; i++)
    {
        if (threads[i] == ctx)
        {
            kfree(VOID_PTR_FROM_U32(ctx->esp - THREAD_STACK_SIZE + 4));
            kfree(ctx);

            threads[i] = threads[thread_count - 1];
            threads[thread_count - 1] = NULL;
            thread_count--;

            TRACK_REMOVE(ctx, ALLOC_SRC_THREAD_CONTEXT);

            return true;
        }
    }

    TRACK_REMOVE(ctx, ALLOC_SRC_THREAD_CONTEXT);
    return false;
}