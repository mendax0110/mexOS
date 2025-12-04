#ifndef KERNEL_DEBUG_UTILS_H
#define KERNEL_DEBUG_UTILS_H

#include "../include/types.h"

#define DEBUG_TRACE_SIZE 64
#define DEBUG_TRACE_MSG_LEN 80

/**
 * @brief Function trace entry structure
 */
typedef struct trace_entry
{
    char function_name[32];
    char message[DEBUG_TRACE_MSG_LEN];
    uint32_t timestamp;
} trace_entry_t;

/**
 * @brief Initialize debug utilities
 */
void debug_utils_init(void);

/**
 * @brief Dump CPU registers to console
 * @param eax EAX register value
 * @param ebx EBX register value
 * @param ecx ECX register value
 * @param edx EDX register value
 * @param esi ESI register value
 * @param edi EDI register value
 * @param ebp EBP register value
 * @param esp ESP register value
 * @param eip EIP register value
 */
void debug_dump_registers(uint32_t eax, uint32_t ebx, uint32_t ecx,
                          uint32_t edx, uint32_t esi, uint32_t edi,
                          uint32_t ebp, uint32_t esp, uint32_t eip);

/**
 * @brief Dump memory region to console
 * @param addr Starting address
 * @param count Number of uint32_t values to dump
 */
void debug_dump_memory(uint32_t* addr, uint32_t count);

/**
 * @brief Add function trace entry
 * @param func_name Function name
 * @param message Trace message
 */
void debug_trace(const char* func_name, const char* message);

/**
 * @brief Print function trace buffer
 */
void debug_print_trace(void);

/**
 * @brief Clear trace buffer
 */
void debug_clear_trace(void);

/**
 * @brief Dump stack contents
 * @param stack_ptr Stack pointer
 * @param count Number of stack entries to dump
 */
void debug_dump_stack(uint32_t* stack_ptr, uint32_t count);

#endif
