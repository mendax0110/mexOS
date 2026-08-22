#include "debug_utils.h"
#include "ui/console.h"
#include "sched/timer.h"
#include "lib/string.h"
#include "include/addr.h"
#include "exec/elf.h"

static trace_entry_t trace_buffer[DEBUG_TRACE_SIZE];
static uint32_t trace_head = 0;
static uint32_t trace_count = 0;

void debug_utils_init(void)
{
    trace_head = 0;
    trace_count = 0;
    memset(trace_buffer, 0, sizeof(trace_buffer));
}

void debug_dump_registers(const uint32_t eax, const uint32_t ebx, const uint32_t ecx,
                          const uint32_t edx, const uint32_t esi, const uint32_t edi,
                          const uint32_t ebp, const uint32_t esp, const uint32_t eip)
{
    console_write("\n=== Register Dump ===\n");
    console_write("EAX: ");
    console_write_hex(eax);
    console_write("  EBX: ");
    console_write_hex(ebx);
    console_write("\nECX: ");
    console_write_hex(ecx);
    console_write("  EDX: ");
    console_write_hex(edx);
    console_write("\nESI: ");
    console_write_hex(esi);
    console_write("  EDI: ");
    console_write_hex(edi);
    console_write("\nEBP: ");
    console_write_hex(ebp);
    console_write("  ESP: ");
    console_write_hex(esp);
    console_write("\nEIP: ");
    console_write_hex(eip);
    console_write("\n\n");
}

void debug_dump_memory(uint32_t* addr, const uint32_t count)
{
    if (!addr)
    {
        console_write("Invalid address\n");
        return;
    }

    console_write("\n=== Memory Dump ===\n");
    console_write("Address: ");
    console_write_hex(PTR_TO_U32(addr));
    console_write("\n\n");

    for (uint32_t i = 0; i < count; i++)
    {
        if (i % 4 == 0)
        {
            console_write_hex(PTR_TO_U32(&addr[i]));
            console_write(": ");
        }

        console_write_hex(addr[i]);
        console_write(" ");

        if ((i + 1) % 4 == 0)
        {
            console_write("\n");
        }
    }
    
    if (count % 4 != 0)
    {
        console_write("\n");
    }
}

void debug_trace(const char* func_name, const char* message)
{
    if (!func_name || !message)
    {
        return;
    }

    trace_entry_t* entry = &trace_buffer[trace_head];
    
    strncpy(entry->function_name, func_name, sizeof(entry->function_name) - 1);
    entry->function_name[sizeof(entry->function_name) - 1] = '\0';
    
    strncpy(entry->message, message, sizeof(entry->message) - 1);
    entry->message[sizeof(entry->message) - 1] = '\0';
    
    entry->timestamp = timer_get_ticks();

    trace_head = (trace_head + 1) % DEBUG_TRACE_SIZE;
    
    if (trace_count < DEBUG_TRACE_SIZE)
    {
        trace_count++;
    }
}

void debug_print_trace(void)
{
    console_write("\n=== Function Trace ===\n\n");

    if (trace_count == 0)
    {
        console_write("(empty)\n");
        return;
    }

    uint32_t start_idx = 0;
    uint32_t entries = trace_count;

    if (trace_count >= DEBUG_TRACE_SIZE)
    {
        start_idx = trace_head;
        entries = DEBUG_TRACE_SIZE;
    }

    for (uint32_t i = 0; i < entries; i++)
    {
        const uint32_t idx = (start_idx + i) % DEBUG_TRACE_SIZE;
        const trace_entry_t* entry = &trace_buffer[idx];

        console_write("[");
        console_write_dec(entry->timestamp);
        console_write("] ");
        console_write(entry->function_name);
        console_write(": ");
        console_write(entry->message);
        console_write("\n");
    }
}

void debug_clear_trace(void)
{
    trace_head = 0;
    trace_count = 0;
    memset(trace_buffer, 0, sizeof(trace_buffer));
    console_write("Trace buffer cleared\n");
}

void debug_dump_stack(uint32_t* stack_ptr, const uint32_t count)
{
    if (!stack_ptr)
    {
        console_write("Invalid stack pointer\n");
        return;
    }

    console_write("\n=== Stack Dump ===\n");
    console_write("Stack pointer: ");
    console_write_hex(PTR_TO_U32(stack_ptr));
    console_write("\n\n");

    for (uint32_t i = 0; i < count; i++)
    {
        console_write("ESP+");
        console_write_dec(i * 4);
        console_write(": ");
        console_write_hex(stack_ptr[i]);
        console_write("\n");
    }
}

const char* debug_get_symbol(const uint32_t addr)
{
    const char* sym = elf_lookup_symbol(addr);
    return sym ? sym : "<unknown>";
}
void __cyg_profile_func_enter(void* this_fn, void* call_site)
{
    const char* func_sym = debug_get_symbol(PTR_TO_U32(this_fn));
    const char* call_sym = debug_get_symbol(PTR_TO_U32(call_site));
    debug_trace(func_sym, call_sym);
}

void __cyg_profile_func_exit(void* this_fn, void* call_site)
{
    UNUSED(call_site, "__cyg_profile_func_exit");
    const char* func_sym = debug_get_symbol(PTR_TO_U32(this_fn));
    debug_trace(func_sym, "<exit>");
}