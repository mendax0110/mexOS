#include "core/rollback.h"

fault_ctx_t* g_fault_ctx = NULL;

void fault_push(fault_ctx_t* ctx)
{
    ctx->prev = g_fault_ctx;
    g_fault_ctx = ctx;
}

void fault_pop(void)
{
    if (g_fault_ctx)
    {
        g_fault_ctx = g_fault_ctx->prev;
    }
}

void rollback_current(void)
{
    if (g_fault_ctx && g_fault_ctx->rollback && !g_fault_ctx->rolled_back)
    {
        g_fault_ctx->rolled_back = true;
        g_fault_ctx->rollback();
    }
}
